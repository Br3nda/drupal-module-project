#!/usr/local/bin/php
<?php

// $Id: package-release-nodes.php,v 1.1.2.7 2006/11/08 01:12:37 dww Exp $

/**
 * @file
 * Automated packaging script to generate tarballs from release nodes.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 *
 * TODO:
 * - better error propagation and robustness
 * - translation stats
 * 
 */

// ------------------------------------------------------------
// Required customization
// ------------------------------------------------------------

// The root of your Drupal installation, so we can properly bootstrap
// Drupal. This should be the full path to the directory that holds
// your index.php file, the "includes" subdirectory, etc.
$drupal_root = '';

// The name of your site. Required so that when we bootstrap Drupal in
// this script, we find the right settings.php file in your sites folder. 
// For example, on drupal.org:
// $site_name = 'drupal.org';
$site_name = '';

// The CVSROOT for the repository this script will be packaging
// releases from.  For example, on drupal.org:
// $cvs_root = ':pserver:anonymous@cvs.drupal.org:/cvs/drupal';
$cvs_root = '';

// Temporary directory where you want packages to be created.
$tmp_dir = '';

// Location of the LICENSE.txt file you want included in all packages.
$license = '';

// Location of the INSTALL.txt file you want included in all
// translation packages.
$trans_install = '';


// ------------------------------------------------------------
// Optional customization
// ------------------------------------------------------------

// ----------------
// File destination
// ----------------
// This assumes you want to install the packaged releases in the
// "files/projects" directory of your root Drupal installation. If
// that's not the case, you should customize these.
$dest_root = $drupal_root;
$dest_rel = 'files/projects';

// --------------
// External tools
// --------------
// If you want this program to always use absolute paths for all the
// tools it invokes, provide a full path for each one. Otherwise,
// the script will find these tools in your PATH.
$tar = '/usr/bin/tar';
$gzip = '/usr/bin/gzip';
$cvs = '/usr/bin/cvs';
$ln = '/bin/ln';
$rm = '/bin/rm';
$msgcat = 'msgcat';
$msgattrib = 'msgattrib';
$msgfmt = 'msgfmt';


// ------------------------------------------------------------
// Initialization
// (Real work begins here, nothing else to customize)
// ------------------------------------------------------------

// Check if all required variables are defined
$vars = array(
  'drupal_root' => $drupal_root,
  'site_name' => $site_name,
  'cvs_root' => $cvs_root,
  'tmp_dir' => $tmp_dir,
  'license' => $license,
  'trans_install' => $trans_install,
);
foreach ($vars as $name => $val) {
  if (empty($val)) {
    print "ERROR: \"\$$name\" variable not set, aborting\n";
    $fatal_err = true;
  }
}
if ($fatal_err) {
  exit(1);
}

putenv("CVSROOT=$cvs_root");
$script_name = $argv[0];

// Find what kind of packaging we need to do
if ($argv[1]) {
  $task = $argv[1];
}
else {
  $task = 'tag';
}
switch($task) {
  case 'tag':
  case 'branch':
    break;
  default:
    print "ERROR: $argv[0] invoked with invalid argument: \"$task\"\n";
    exit (1);
}

// Setup variables for Drupal bootstrap
$_SERVER['HTTP_HOST'] = $site_name;
$_SERVER['REQUEST_URI'] = '/' . $script_name;
$_SERVER['SCRIPT_NAME'] = '/' . $script_name;
$_SERVER['PHP_SELF'] = '/' . $script_name;
$_SERVER['SCRIPT_FILENAME'] = $_SERVER['PWD'] . '/' . $script_name;
$_SERVER['PATH_TRANSLATED'] = $_SERVER['SCRIPT_FILENAME'];

if (!chdir($drupal_root)) {
  print "ERROR: Can't chdir($drupal_root): aborting.\n";
  exit(1);
}

require_once 'includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

initialize_repository_info();

package_releases($task);


// ------------------------------------------------------------
// Functions: main work
// ------------------------------------------------------------

function package_releases($type) {
  if ($type == 'tag') {
    $where = " AND prn.rebuild = 0 AND (prn.file_path IS NULL OR prn.file_path = '')";
    $plural = 'tags';
    $check_new = false;
  }
  elseif ($type == 'branch') {
    $where = " AND prn.rebuild = 1";
    $plural = 'branches';
    $check_new = true;
  }
  else {
    watchdog('release_package', t("ERROR: package_releases() called with unknown type: %type", array('%type' => theme('placeholder', $type))));
    return;
  }

  watchdog('release_package', t("Starting to package all releases from $plural."));

  $query = db_query("SELECT pp.uri, prn.nid, prn.tag, prn.version, c.directory, c.rid FROM {project_release_nodes} prn INNER JOIN {project_projects} pp ON prn.pid = pp.nid INNER JOIN {node} np ON prn.pid = np.nid INNER JOIN {project_release_projects} prp ON prp.nid = prn.pid INNER JOIN {cvs_projects} c ON prn.pid = c.nid WHERE np.status = 1 AND prp.releases = 1" . $where);

  $num_built = 0;
  $num_considered = 0;
  while ($release = db_fetch_object($query)) {
    $id = $release->uri . '-' . $release->version;
    $tag = $release->tag;
    $nid = $release->nid;
    $rev = ($tag == 'TRUNK' || $tag == 'HEAD') ? '-A' : "-r $tag";
    watchdog('release_package', t("Working on %type release: %id from $type: %tag", array('%type' => $release->rid == 1 ? t('core') : t('contrib'), '%id' => theme_placeholder($id), '%tag' => theme_placeholder($tag))));
    if ($release->rid == 1) {
      $built = package_release_core($nid, $id, $rev, $check_new);
    }
    else {
      $built = package_release_contrib($nid, $id, $rev, $release->directory, $check_new);
    }
    if ($built) {
      $num_built++;
    }
    $num_considered++;
  }
  watchdog('release_package', t("Done packaging releases from $plural: %num_built built, %num_considered considered.", array('%num_built' => $num_built, '%num_considered' => $num_considered)));
}

function package_release_core($nid, $id, $rev, $check_new) {
  global $tmp_dir, $repositories, $dest_root, $dest_rel;
  global $cvs, $tar, $gzip, $rm;
  $rid = 1;

  if (!chdir($tmp_dir)) {
    watchdog('release_package', t("ERROR: Can't chdir(%dir)", array('%dir' => $tmp_dir)));
    return false;
  }

  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  // Actually generate the tarball:
  `$cvs -q co $rev -Pd $id drupal`;
  // TODO: do we even care about $check_new?  The old script didn't...
  `$tar -c --exclude=CVS --file=- $id | $gzip -9 --no-name > $full_dest`;

  `$rm -rf $tmp_dir/$id`;
  // TODO: need better error checking

  package_release_update_node($nid, $file_path);
  return true;
}

function package_release_contrib($nid, $id, $rev, $dir, $check_new) {
  global $tmp_dir, $repositories, $dest_root, $dest_rel;
  global $cvs, $tar, $gzip, $rm, $ln;
  global $msgcat, $msgattrib, $msgfmt;
  global $license, $trans_install;
  $rid = 2;
  // Files to ignore when checking timestamps:
  $exclude = array('.', '..', 'CVS', 'LICENSE.txt');

  $parts = split('/', $dir);
  // modules, themes, theme-engines, or translations
  $contrib_type = $parts[1];
  // specific directory (same as uri)
  $uri = $parts[2];

  $basedir = $repositories[$rid]['modules'] . '/' . $contrib_type;
  $fulldir = $basedir . '/' . $uri;
  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  if (!chdir($tmp_dir)) {
    watchdog('release_package', t("ERROR: Can't chdir(%dir)", array('%dir' => $tmp_dir)));
    return false;
  }

  // Checkout this release from CVS, and see if we need to rebuild it
  `$cvs -q co $rev $fulldir`;
  chdir($basedir);
  if ($contrib_type == 'translations') {
    $exclude = array_merge($exclude, 'README.txt');
  }
  if (is_file($full_dest) && $check_new) {
    $youngest = file_find_youngest($uri, 0, $exclude);
    if (filectime($full_dest) + 300 > $youngest) {
      // The existing tarball for this release is newer than the youngest
      // file in the directory, we're done.
      watchdog('release_package', t("%id is unchanged, not re-packaging", array('%id' => theme('placeholder', $id))));
      return false;
    }
  }

  // Link not copy, since we want to preserve the date...
  `$ln -sf $license $uri/LICENSE.txt`;
  if ($contrib_type == 'translations' && $uri != 'drupal-pot') {
    if ($handle = opendir($uri)) {
      $po_files = array();
      while ($file = readdir($handle)) {
        if ($file == 'general.po') {
          $found_general_po = true;
        }
        elseif ($file == 'installer.po') {
          $found_installer_po = true;
        }
        elseif (preg_match('/.*\.po/', $file)) {
          $po_files[] = "$uri/$file";
        }
      }
      if ($found_general_po) {
        @unlink("$uri/$uri.po");
        $po_targets = "$uri/general.po ";
        $po_targets .= implode(' ', $po_files);
        `$msgcat $po_targets | $msgattrib --no-fuzzy -o $uri/$uri.po`;
      }
    }
    if (is_file("$uri/$uri.po")) {
      `$msgfmt --statistics $uri/$uri.po 2>> $uri/README.txt`;
      $to_tar = "$uri/*.txt $uri/$uri.po";
      if ($found_installer_po) {
        $to_tar .= " $uri/installer.po";
      }
    }
    else {
      watchdog('release_package', t("ERROR: %uri translation does not contain a %uri_po file, not packaging", array('%uri' => theme('placeholder', $uri), '%uri_po' => theme('placeholder', "$uri.po"))));
      return false;
    }
  }
  else {
    // NOT a translation: no special packaging, grab the whole directory.
    $to_tar = $uri;
  }

  // 'h' is for dereference, we want to include the files, not the links
  `$tar -ch --exclude=CVS --file=- $to_tar | $gzip -9 --no-name > $full_dest`;

  `$rm -rf $tmp_dir/$basedir/$uri`;
  // TODO: need better error checking

  package_release_update_node($nid, $file_path);
}

// ------------------------------------------------------------
// Functions: utility methods
// ------------------------------------------------------------

/// TODO: remove this before the final script goes live -- debugging only.
function wprint($var) {
  watchdog('package_debug', '<pre>' . var_export($var, TRUE));
}

/**
 * Initialize info from the {cvs_repositories} table, since there are
 * usually only a tiny handful of records, and it'll be faster to do
 * whatever we need via php than another JOIN...
 */
function initialize_repository_info() {
  global $repositories;
  $query = db_query("SELECT rid, root, modules, name FROM {cvs_repositories}");
  while ($repo = db_fetch_object($query)) {
    $repositories[$repo->rid] = array('root' => $repo->root, 'modules' => $repo->modules, 'name' => $repo->name);
  }
}

/**
 * Update the DB with the new file info for a given release node.
 */
function package_release_update_node($nid, $file_path) {
  global $dest_root;
  $full_path = $dest_root . '/' . $file;

  // Now that we have the official file, compute some metadata:
  $file_date = filemtime($full_path);
  $file_hash = md5_file($full_path);

  // Finally, update the node in the DB about this file:
  db_query("UPDATE {project_release_nodes} SET file_path = '%s', file_hash = '%s', file_date = %d WHERE nid = %d", $file_path, $file_hash, $file_date, $nid);
  db_query("UPDATE {node} SET status = 1 WHERE nid = %d", $nid);
}

/**
 * Find the youngest (newest) file in a directory tree.
 * Stolen wholesale from the original package-drupal.php script.
 */
function file_find_youngest($dir, $timestamp, $exclude) {
  if (is_dir($dir)) {
    $fp = opendir($dir);
    while (FALSE !== ($file = readdir($fp))) {
      if (!in_array($file, $exclude)) {
        if (is_dir("$dir/$file")) {
          $timestamp = file_find_youngest("$dir/$file", $timestamp, $exclude);
        }
        else {
          $timestamp = (filectime("$dir/$file") > $timestamp) ? filectime("$dir/$file") : $timestamp;
        }
      }
    }
    closedir($fp);
  }
  return $timestamp;
}


// ------------------------------------------------------------
// Functions: translation-status-related methods
// TODO: get all this working. ;)
// ------------------------------------------------------------


/**
 * Extract some translation statistics:
 */
function translation_status($dir, $version) {
  global $translations;

  $number_of_strings = translation_number_of_strings('drupal-pot', $version);

  $line = exec("$msgfmt --statistics $dir/$dir.po 2>&1");
  $words = preg_split('[\s]', $line, -1, PREG_SPLIT_NO_EMPTY);

  if (is_numeric($words[0]) && is_numeric($number_of_strings)) {
    $percentage = floor((100 * $words[0]) / ($number_of_strings));
    if ($percentage >= 100) {
      $translations[$dir][$version] = "<td style=\"color: green; font-weight: bold;\">100% (complete)</td>";
    }
    else {
      $translations[$dir][$version] = "<td>". $percentage ."% (". ($number_of_strings - $words[0]). " missing)</td>";
    }
  }
  else {
    $translations[$dir][$version] = "<td style=\"color: red; font-weight: bold;\">translation broken</td>";
  }
}

function translation_report($versions) {
  global $dest, $translations;

  $output  = "<table>\n";
  $output .= " <tr><th>Language</th>";
  foreach ($versions as $version) {
    $output .= "<th>$version</th>";
  }
  $output .= " </tr>\n";

  ksort($translations);
  foreach ($translations as $language => $data) {
    $output .= " <tr><td><a href=\"project/$language\">$language</a></td>";
    foreach ($versions as $version) {
      if ($data[$version]) {
        $output .= $data[$version];
      }
      else {
        $output .= "<td></td>";
      }
    }
    $output .= "</tr>\n";
  }
  $output .= "</table>";

  $fd = fopen("$dest/translation-status.txt", 'w');
  fwrite($fd, $output);
  fclose($fd);
  wprint("wrote $dest/translation-status.txt");
}

function translation_number_of_strings($dir, $version) {
  static $number_of_strings = array();
  if (!isset($number_of_strings[$version])) {
    `$msgcat $dir/general.pot $dir/[^g]*.pot | $msgattrib --no-fuzzy -o $dir/$dir.pot`;
    $line = exec("$msgfmt --statistics $dir/$dir.pot 2>&1");
    $words = preg_split('[\s]', $line, -1, PREG_SPLIT_NO_EMPTY);
    $number_of_strings[$version] = $words[3];
    @unlink("$dir/$dir.pot");
  }
  return $number_of_strings[$version];
}
