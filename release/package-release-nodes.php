#!/usr/bin/php
<?php

// $Id: package-release-nodes.php,v 1.28.2.2 2008/10/29 00:19:52 dww Exp $

/**
 * @file
 * Automated packaging script to generate tarballs from release nodes.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 *
 * TODO:
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

// Root of the temporary directory where you want packages to be
// made. Subdirectories will be created depending on the task.
$tmp_root = '';

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

// The taxonomy id (tid) of the "Security update" term on drupal.org
define('SECURITY_UPDATE_TID', 100);

// ------------------------------------------------------------
// Initialization
// (Real work begins here, nothing else to customize)
// ------------------------------------------------------------

// Check if all required variables are defined
$vars = array(
  'drupal_root' => $drupal_root,
  'site_name' => $site_name,
  'cvs_root' => $cvs_root,
  'tmp_root' => $tmp_root,
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
  case 'check':
  case 'repair':
    break;
  default:
    print "ERROR: $argv[0] invoked with invalid argument: \"$task\"\n";
    exit (1);
}

$project_id = $argv[2];

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

// Force the right umask while this script runs, so that everything is created
// with sane file permissions.
umask(0022);

require_once 'includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

if ($task == 'check' || $task == 'repair') {
  verify_packages($task, $project_id);
}
else {
  initialize_tmp_dir($task);
  initialize_repository_info();
  package_releases($task, $project_id);
  // Now that we're done, clean out the tmp/task dir we created
  chdir($tmp_root);
  drupal_exec("$rm -rf $tmp_dir");
}

if ($task == 'branch') {
  // Clear any cached data set to expire.
  cache_clear_all(NULL, 'cache_project_release');
}
elseif ($task == 'repair') {
  // Clear all cached data
  cache_clear_all('*', 'cache_project_release', TRUE);
}

// ------------------------------------------------------------
// Functions: main work
// ------------------------------------------------------------

function package_releases($type, $project_id) {
  global $wd_err_msg;
  if ($type == 'tag') {
    $where = " AND (prn.rebuild = 0) AND (prn.file_path = '')";
    $plural = t('tags');
  }
  elseif ($type == 'branch') {
    $rel_node_join = " INNER JOIN {node} nr ON prn.nid = nr.nid";
    $where = " AND (prn.rebuild = 1) AND ((prn.file_path = '') OR (nr.status = 1))";
    $plural = t('branches');
    if (empty($project_id)) {
      wd_msg(t("Starting to package all snapshot releases."));
    }
    else {
      wd_msg(t("Starting to package snapshot releases for project id: @project_id.", array('@project_id' => $project_id)), l(t('view'), 'node/' . $project_id));
    }
  }
  else {
    wd_err(t("ERROR: package_releases() called with unknown type: %type", array('%type' => $type)));
    return;
  }
  $args = array();
  if (!empty($project_id)) {
    $where .= ' AND prn.pid = %d';
    $args[] = $project_id;
  }
  $vid = (int)_project_release_get_api_vid();

  $query = db_query("SELECT pp.uri, prn.nid, prn.pid, prn.tag, prn.version, prn.version_major, td.tid, c.directory, c.rid FROM {project_release_nodes} prn $rel_node_join INNER JOIN {term_node} tn ON prn.nid = tn.nid INNER JOIN {term_data} td ON tn.tid = td.tid INNER JOIN {project_projects} pp ON prn.pid = pp.nid INNER JOIN {node} np ON prn.pid = np.nid INNER JOIN {project_release_projects} prp ON prp.nid = prn.pid INNER JOIN {cvs_projects} c ON prn.pid = c.nid WHERE np.status = 1 AND prp.releases = 1 AND td.vid = $vid " . $where . ' ORDER BY pp.uri', $args);

  $num_built = 0;
  $num_considered = 0;
  $project_nids = array();

  // Read everything out of the query immediately so that we don't leave the
  // query object/connection open while doing other queries.
  $releases = array();
  while ($release = db_fetch_object($query)) {
    $releases[] = $release;
  }
  foreach ($releases as $release) {
    $wd_err_msg = array();
    $version = $release->version;
    $uri = $release->uri;
    $tag = $release->tag;
    $nid = $release->nid;
    $pid = $release->pid;
    $tid = $release->tid;
    $major = $release->version_major;
    $rev = ($tag == 'TRUNK') ? '-r HEAD' : "-r $tag";
    $uri = escapeshellcmd($uri);
    $version = escapeshellcmd($version);
    $rev = escapeshellcmd($rev);
    db_query("DELETE FROM {project_release_package_errors} WHERE nid = %d", $nid);
    if ($release->rid == 1) {
      $built = package_release_core($nid, $uri, $version, $rev);
    }
    else {
      $dir = escapeshellcmd($release->directory);
      $built = package_release_contrib($nid, $uri, $version, $rev, $dir);
    }
    if ($built) {
      $num_built++;
      $project_nids[$pid][$tid][$major] = TRUE;
    }
    $num_considered++;
    if (count($wd_err_msg)) {
      db_query("INSERT INTO {project_release_package_errors} (nid, messages) values (%d, '%s')", $nid, serialize($wd_err_msg));
    }
  }
  if ($num_built || $type == 'branch') {
    if (!empty($project_id)) {
      wd_msg(t("Done packaging releases for @uri from !plural: !num_built built, !num_considered considered.", array('@uri' => $uri, '!plural' => $plural, '!num_built' => $num_built, '!num_considered' => $num_considered)));
    }
    else {
      wd_msg(t("Done packaging releases from !plural: !num_built built, !num_considered considered.", array('!plural' => $plural, '!num_built' => $num_built, '!num_considered' => $num_considered)));
    }
  }

  // Finally, for each project/tid/major triple we packaged, check to see if
  // the supported/recommended settings are sane now that new tarballs have
  // been generated and release nodes published.
  foreach ($project_nids as $pid => $tids) {
    foreach ($tids as $tid => $majors) {
      foreach ($majors as $major => $value) {
        project_release_check_supported_versions($pid, $tid, $major, FALSE);
      }
    }
  }
}

function package_release_core($nid, $uri, $version, $rev) {
  global $tmp_dir, $repositories, $dest_root, $dest_rel;
  global $cvs, $tar, $gzip, $rm;
  $rid = 1;

  if (!drupal_chdir($tmp_dir)) {
    return false;
  }

  $id = $uri . '-' . $version;
  $view_link = l(t('view'), 'node/' . $nid);
  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  // Don't use drupal_exec or return if this fails, we expect it to be empty.
  exec("$rm -rf $tmp_dir/$id");

  // Actually generate the tarball:
  if (!drupal_exec("$cvs -q export $rev -d $id drupal")) {
    return false;
  }

  $info_files = array();
  $exclude = array('.', '..', 'LICENSE.txt');
  $youngest = file_find_youngest($id, 0, $exclude, $info_files);
  if (is_file($full_dest) && filectime($full_dest) + 300 > $youngest) {
    // The existing tarball for this release is newer than the youngest
    // file in the directory, we're done.
    return false;
  }

  // Fix any .info files
  foreach ($info_files as $file) {
    if (!fix_info_file_version($file, $uri, $version)) {
      wd_err(t("ERROR: Failed to update version in %file, aborting packaging", array('%file' => $file)), $view_link);
      return false;
    }
  }

  if (!drupal_exec("$tar -c --file=- $id | $gzip -9 --no-name > $full_dest")) {
    return false;
  }

  // As soon as the tarball exists, we want to update the DB about it.
  package_release_update_node($nid, $file_path);

  wd_msg(t("%id has changed, re-packaged.", array('%id' => $id)), $view_link);

  // Don't consider failure to remove this directory a build failure.
  drupal_exec("$rm -rf $tmp_dir/$id");
  return true;
}

function package_release_contrib($nid, $uri, $version, $rev, $dir) {
  global $tmp_dir, $repositories, $dest_root, $dest_rel;
  global $cvs, $tar, $gzip, $rm, $ln;
  global $license, $trans_install;

  $rid = 2;
  // Files to ignore when checking timestamps:
  $exclude = array('.', '..', 'LICENSE.txt');

  $parts = split('/', $dir);
  // modules, themes, theme-engines, or translations
  $contrib_type = $parts[1];
  // specific directory (same as uri)
  $uri = $parts[2];

  $id = $uri . '-' . $version;
  $view_link = l(t('view'), 'node/' . $nid);
  $basedir = $repositories[$rid]['modules'] . '/' . $contrib_type;
  $fulldir = $basedir . '/' . $uri;
  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  if (!drupal_chdir($tmp_dir)) {
    return false;
  }

  // Don't use drupal_exec or return if this fails, we expect it to be empty.
  exec("$rm -rf $tmp_dir/$fulldir");

  // Checkout this release from CVS, and see if we need to rebuild it
  if (!drupal_exec("$cvs -q export $rev $fulldir")) {
    return false;
  }
  if (!is_dir($fulldir)) {
    wd_err(t("ERROR: %dir does not exist after cvs export %rev", array('%dir' => $fulldir, '%rev' =>  $rev)), $view_link);
    return false;
  }
  if (!drupal_chdir($basedir)) {
    // TODO: try to clean up the cvs export we just did?
    // seems scary if we can't even chdir($basedir)...
    return false;
  }

  $info_files = array();
  $youngest = file_find_youngest($uri, 0, $exclude, $info_files);
  if (is_file($full_dest) && filectime($full_dest) + 300 > $youngest) {
    // The existing tarball for this release is newer than the youngest
    // file in the directory, we're done.
    return false;
  }

  // Fix any .info files
  foreach ($info_files as $file) {
    if (!fix_info_file_version($file, $uri, $version)) {
      wd_err(t("ERROR: Failed to update version in %file, aborting packaging", array('%file' => $file)), $view_link);
      return false;
    }
  }

  // Link not copy, since we want to preserve the date...
  if (!drupal_exec("$ln -sf $license $uri/LICENSE.txt")) {
    return false;
  }
  // Do we want a subdirectory in the tarball or not?
  $tarball_needs_subdir = TRUE;
  if ($contrib_type == 'translations' && $uri != 'drupal-pot') {
    // Translation projects are packaged differently based on core version.
    if (intval($version) == 6) {
      if (!($to_tar = package_release_contrib_d6_translation($uri, $version, $view_link))) {
        // Return on error.
        return FALSE;
      }
      $tarball_needs_subdir = FALSE;
    }
    elseif (!($to_tar = package_release_contrib_pre_d6_translation($uri, $version, $view_link))) {
      // Return on error.
      return FALSE;
    }
  }
  else {
    // Not a translation: just grab the whole directory.
    $to_tar = $uri;
  }

  if (!$tarball_needs_subdir) {
    if (!drupal_chdir($uri)) {
      return false;
    }
  }

  // 'h' is for dereference, we want to include the files, not the links
  if (!drupal_exec("$tar -ch --file=- $to_tar | $gzip -9 --no-name > $full_dest")) {
    return false;
  }

  // As soon as the tarball exists, update the DB
  package_release_update_node($nid, $file_path);

  wd_msg(t("%id has changed, re-packaged.", array('%id' => $id)), $view_link);

  // Don't consider failure to remove this directory a build failure.
  drupal_exec("$rm -rf $tmp_dir/$basedir/$uri");
  return true;
}

function package_release_contrib_pre_d6_translation($uri, $version, $view_link) {
  global $msgcat, $msgattrib, $msgfmt;

  if ($handle = opendir($uri)) {
    $po_files = array();
    while ($file = readdir($handle)) {
      if ($file == 'general.po') {
        $found_general_po = TRUE;
      }
      elseif ($file == 'installer.po') {
        $found_installer_po = TRUE;
      }
      elseif (preg_match('/.*\.po/', $file)) {
        $po_files[] = "$uri/$file";
      }
    }
    if ($found_general_po) {
      @unlink("$uri/$uri.po");
      $po_targets = "$uri/general.po ";
      $po_targets .= implode(' ', $po_files);
      if (!drupal_exec("$msgcat --use-first $po_targets | $msgattrib --no-fuzzy -o $uri/$uri.po")) {
        return FALSE;
      }
    }
  }
  if (is_file("$uri/$uri.po")) {
    if (!drupal_exec("$msgfmt --statistics $uri/$uri.po 2>> $uri/STATUS.txt")) {
      return FALSE;
    }
    $to_tar = "$uri/*.txt $uri/$uri.po";
    if ($found_installer_po) {
      $to_tar .= " $uri/installer.po";
    }
  }
  else {
    wd_err(t("ERROR: %uri translation does not contain a %uri_po file for version %version, not packaging", array('%uri' => $uri, '%uri_po' => "$uri.po", '%version' => $version)), $view_link);
    return FALSE;
  }

  // Return with list of files to package.
  return $to_tar;
}

function package_release_contrib_d6_translation($uri, $version, $view_link) {
  global $msgattrib, $msgfmt;

  if ($handle = opendir($uri)) {
    $po_files = array();
    while ($file = readdir($handle)) {
      if (preg_match('!(.*)\.txt$!', $file, $name) && ($file != "STATUS.$uri.txt")) {
        // Rename text files to $name[1].$uri.txt so there will be no conflict
        // with core text files when the package is deployed.
        if (!rename("$uri/$file", "$uri/$name[1].$uri.txt")) {
          wd_err(t("ERROR: Unable to rename text files in %uri translation in version %version, not packaging", array('%uri' => $uri, '%version' => $version)), $view_link);
          return FALSE;
        }
      }
      elseif (preg_match('!.*\.po$!', $file)) {

        // Generate stats information about the .po file handled.
        if (!drupal_exec("$msgfmt --statistics $uri/$file 2>> $uri/STATUS.$uri.txt")) {
          wd_err(t("ERROR: Unable to generate statistics for %file in %uri translation in version %version, not packaging", array('%uri' => $uri, '%version' => $version, '%file' => $file)), $view_link);
          return FALSE;
        }

        // File names are formatted in directory-subdirectory.po or
        // directory.po format and aggregate files from the named directory.
        // The installer.po file is special in that it aggregates all strings
        // possibly used in the installer. We move that to the default install
        // profile. We move all other root directory files (misc.po,
        // includes.po, etc) to the system module and all remaining files to
        // the corresponding subdirectory in the named directory.
        if (!strpos($file, '-')) {
          if ($file == 'installer.po') {
            // Special file, goes to install profile.
            $target = 'profiles/default/translations/'. $uri .'.po';
          }
          else {
            // 'Root' files go to system module.
            $target = 'modules/system/translations/'. str_replace('.po', '.'. $uri .'.po', $file);
          }
        }
        else {
          // Other files go to their module or theme folder.
          $target = str_replace(array('-', '.po'), array('/', ''), $file) .'/translations/'. str_replace('.po', '.'. $uri .'.po', $file);
        }
        $uri_target = "$uri/$target";

        // Create target folder and copy file there, while removing fuzzies.
        $target_dir = dirname($uri_target);
        if (!is_dir($target_dir) && !mkdir($target_dir, 0777, TRUE)) {
          wd_err(t("ERROR: Unable to generate directory structure in %uri translation in version %version, not packaging", array('%uri' => $uri, '%version' => $version)), $view_link);
          return FALSE;
        }
        if (!drupal_exec("$msgattrib --no-fuzzy $uri/$file -o $uri_target")) {
          wd_err(t("ERROR: Unable to filter fuzzy strings and copying the translation files in %uri translation in version %version, not packaging", array('%uri' => $uri, '%version' => $version)), $view_link);
          return FALSE;
        }
      
        // Add file to package.
        $to_tar .= ' '. $target;
      }
    }
  }

  // Return with list of files to package.
  return "*.txt". $to_tar;
}

// ------------------------------------------------------------
// Functions: metadata validation functions
// ------------------------------------------------------------

/**
 * Check that file metadata on disk matches the values stored in the DB.
 */
function verify_packages($task, $project_id) {
  global $dest_root;
  $do_repair = $task == 'repair' ? TRUE : FALSE;
  $args = array();
  $where = '';
  if (!empty($project_id)) {
    $where = ' AND prn.pid = %d';
    $args[] = $project_id;
  }
  $query = db_query("SELECT prn.nid, prn.file_path, prn.file_date, prn.file_hash FROM {project_release_nodes} prn INNER JOIN {node} n ON prn.nid = n.nid WHERE n.status = 1 AND prn.file_path <> '' $where", $args);
  while ($release = db_fetch_object($query)) {
    // Grab all the results into RAM to free up the DB connection for
    // when we need to update the DB to correct metadata or log messages.
    $releases[] = $release;
  }

  $num_failed = 0;
  $num_repaired = 0;
  $num_not_exist = 0;
  $num_need_repair = 0;
  $num_considered = 0;
  $num_wrong_date = 0;
  $num_wrong_hash = 0;

  // Now, process the files, and check metadata
  foreach ($releases as $release) {
    $valid_hash = TRUE;
    $valid_date = TRUE;
    $num_considered++;
    $nid = $release->nid;
    $view_link = l(t('view'), 'node/' . $nid);
    $file_path = $release->file_path;
    $full_path = $dest_root . '/' . $file_path;
    $db_date = (int)$release->file_date;
    $db_hash = $release->file_hash;

    if (!is_file($full_path)) {
      $num_not_exist++;
      wd_err(t('WARNING: %file does not exist.', array('%file' => $full_path)), $view_link);
      continue;
    }
    $real_date = filemtime($full_path);
    $real_hash = md5_file($full_path);

    $variables = array();
    $variables['%file'] = $file_path;
    if ($real_hash != $db_hash) {
      $valid_hash = FALSE;
      $num_wrong_hash++;
      $variables['@db_hash'] = $db_hash;
      $variables['@real_hash'] = $real_hash;
    }
    if ($real_date != $db_date) {
      $valid_date = FALSE;
      $num_wrong_date++;
      $variables['!db_date'] = format_date($db_date);
      $variables['!db_date_raw'] = $db_date;
      $variables['!real_date'] = format_date($real_date);
      $variables['!real_date_raw'] = $real_date;
    }
    if ($valid_date && $valid_hash) {
      // Nothing else to do.
      continue;
    }

    if (!$valid_date && !$valid_hash) {
      wd_check(t('All file meta data for %file is incorrect: saved date: !db_date (!db_date_raw), real date: !real_date (!real_date_raw); saved md5hash: @db_hash, real md5hash: @real_hash', $variables), $view_link);
    }
    else if (!$valid_date) {
      wd_check(t('File date for %file is incorrect: saved date: !db_date (!db_date_raw), real date: !real_date (!real_date_raw)', $variables), $view_link);
    }
    else { // !$valid_hash
      wd_check(t('File md5hash for %file is incorrect: saved: @db_hash, real: @real_hash', $variables), $view_link);
    }

    if (!$do_repair) {
      $num_need_repair++;
    }
    else if (!db_query("UPDATE {project_release_nodes} SET file_hash = '%s', file_date = %d WHERE nid = %d", $real_hash, $real_date, $nid)) {
      wd_err(t('ERROR: db_query() failed trying to update metadata for %file', array('%file' => $file_path)), $view_link);
      $num_failed++;
    }
    else {
      $num_repaired++;
    }
  }

  $num_vars = array(
    '!num_considered' => $num_considered,
    '!num_repaired' => $num_repaired,
    '!num_need_repair' => $num_need_repair,
    '!num_wrong_date' => $num_wrong_date,
    '!num_wrong_hash' => $num_wrong_hash,
  );
  if ($num_failed) {
    wd_err(t('ERROR: unable to repair !num_failed releases due to db_query() failures.', array('!num_failed' => $num_failed)));
  }
  if ($num_not_exist) {
    wd_err(t('ERROR: !num_not_exist files are in the database but do not exist on disk.', array('!num_not_exist' => $num_not_exist)));
  }
  if ($do_repair) {
    wd_check(t('Done checking releases: !num_repaired repaired, !num_wrong_date invalid dates, !num_wrong_hash invalid md5 hashes, !num_considered considered.', $num_vars));
  }
  else {
    if (empty($project_id)) {
      wd_check(t('Done checking releases: !num_need_repair need repairing, !num_wrong_date invalid dates, !num_wrong_hash invalid md5 hashes, !num_considered considered.', $num_vars));
    }
    else {
      $num_vars['@project_id'] = $project_id;
      wd_check(t('Done checking releases for project id @project_id: !num_need_repair need repairing, !num_wrong_date invalid dates, !num_wrong_hash invalid md5 hashes, !num_considered considered.', $num_vars), l(t('view'), 'node/' . $project_id));
    }
  }
}

// ------------------------------------------------------------
// Functions: utility methods
// ------------------------------------------------------------

/**
 * Wrapper for exec() that logs errors to the watchdog.
 * @param $cmd
 *   String of the command to execute (assumed to be safe, the caller is
 *   responsible for calling escapeshellcmd() if necessary).
 * @return true if the command was successful (0 exit status), else false.
 */
function drupal_exec($cmd) {
  // Made sure we grab stderr, too...
  exec("$cmd 2>&1", $output, $rval);
  if ($rval) {
    wd_err(t("ERROR: %cmd failed with status !rval", array('%cmd' => $cmd, '!rval' => $rval)) . '<pre>' . implode("\n", array_map('htmlspecialchars', $output)));
    return false;
  }
  return true;
}

/**
 * Wrapper for chdir() that logs errors to the watchdog.
 * @param $dir Directory to change into.
 * @return true if the command was successful (0 exit status), else false.
 */
function drupal_chdir($dir) {
  if (!chdir($dir)) {
    wd_err(t("ERROR: Can't chdir(@dir)", array('@dir' => $dir)));
    return false;
  }
  return true;
}

/// TODO: remove this before the final script goes live -- debugging only.
function wprint($var) {
  watchdog('package_debug', '<pre>' . var_export($var, TRUE));
}

/**
 * Wrapper function for watchdog() to log notice messages. Uses a
 * different watchdog message type depending on the task (branch vs. tag).
 */
function wd_msg($msg, $link = NULL) {
  global $task;
  watchdog('package_' . $task, $msg, WATCHDOG_NOTICE, $link);
  echo $msg ."\n";
}

/**
 * Wrapper function for watchdog() to log error messages.
 */
function wd_err($msg, $link = NULL) {
  global $wd_err_msg;
  if (!isset($wd_err_msg)) {
    $wd_err_msg = array();
  }
  watchdog('package_error', $msg, WATCHDOG_ERROR, $link);
  echo $msg ."\n";
  $wd_err_msg[] = $msg;
}

/**
 * Wrapper function for watchdog() to log messages about checking
 * package metadata.
 */
function wd_check($msg, $link = NULL) {
  watchdog('package_check', $msg, WATCHDOG_NOTICE, $link);
  echo $msg ."\n";
}

/**
 * Initialize the tmp directory. Use different subdirs for building
 * snapshots than official tags, so there's no potential directory
 * collisions and race conditions if both are running at the same time
 * (due to how long it takes to complete a branch snapshot run, and
 * how often we run this for tag-based releases).
 */
function initialize_tmp_dir($task) {
  global $tmp_dir, $tmp_root;

  if (!is_dir($tmp_root)) {
    wd_err(t("ERROR: tmp_root: @dir is not a directory", array('@dir' => $tmp_root)));
    exit(1);
  }

  $tmp_dir = $tmp_root . '/' . $task;
  if (is_dir($tmp_dir)) {
    // Make sure we start with a clean slate
    drupal_exec("$rm -rf $tmp_dir/*");
  }
  else if (!@mkdir($tmp_dir)) {
    wd_err(t("ERROR: mkdir(@dir) failed", array('@dir' => $tmp_dir)));
    exit(1);
  }
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
 * Fix the given .info file with the specified version string
 */
function fix_info_file_version($file, $uri, $version) {
  global $site_name;

  $info = "\n; Information added by $site_name packaging script on " . date('Y-m-d') . "\n";
  $info .= "version = \"$version\"\n";
  // .info files started with 5.x, so we don't have to worry about version
  // strings like "4.7.x-1.0" in this regular expression. If we can't parse
  // the version (also from an old "HEAD" release), or the version isn't at
  // least 6.x, don't add any "core" attribute at all.
  $matches = array();
  if (preg_match('/^((\d+)\.x)-.*/', $version, $matches) && $matches[2] >= 6) {
    $info .= "core = \"$matches[1]\"\n";
  }
  $info .= "project = \"$uri\"\n";
  $info .= 'datestamp = "'. time() ."\"\n";
  $info .= "\n";

  if (!chmod($file, 0644)) {
    wd_err(t("ERROR: chmod(@file, 0644) failed", array('@file' => $file)));
    return false;
  }
  if (!$info_fd = fopen($file, 'ab')) {
    wd_err(t("ERROR: fopen(@file, 'ab') failed", array('@file' => $file)));
    return false;
  }
  if (!fwrite($info_fd, $info)) {
    wd_err(t("ERROR: fwrite(@file) failed", array('@file' => $file)) . '<pre>' . $info);
    return false;
  }
  return true;
}


/**
 * Update the DB with the new file info for a given release node.
 */
function package_release_update_node($nid, $file_path) {
  global $dest_root, $task;
  $full_path = $dest_root . '/' . $file_path;

  // PHP will cache the results of stat() and give us stale answers
  // here, unless we manually tell it otherwise!
  clearstatcache();

  // Now that we have the official file, compute some metadata:
  $file_date = filemtime($full_path);
  $file_hash = md5_file($full_path);

  // Finally, update the node in the DB about this file:
  db_query("UPDATE {project_release_nodes} SET file_path = '%s', file_hash = '%s', file_date = %d WHERE nid = %d", $file_path, $file_hash, $file_date, $nid);

  // Don't auto-publish security updates.
  if ($task == 'tag' && db_num_rows(db_query("SELECT * FROM {term_node} WHERE nid = %d AND tid = %d", $nid, SECURITY_UPDATE_TID))) {
    watchdog('package_security', t("Not auto-publishing security update release."), WATCHDOG_NOTICE, l(t('view'), 'node/'. $nid));
    return;
  }

  db_query("UPDATE {node} SET status = 1 WHERE nid = %d", $nid);
}

/**
 * Find the youngest (newest) file in a directory tree.
 * Stolen wholesale from the original package-drupal.php script.
 * Modified to also notice any files that end with ".info" and store
 * all of them in the array passed in as an argument. Since we have to
 * recurse through the whole directory tree already, we should just
 * record all the info we need in one pass instead of doing it twice.
 */
function file_find_youngest($dir, $timestamp, $exclude, &$info_files) {
  if (is_dir($dir)) {
    $fp = opendir($dir);
    while (FALSE !== ($file = readdir($fp))) {
      if (!in_array($file, $exclude)) {
        if (is_dir("$dir/$file")) {
          $timestamp = file_find_youngest("$dir/$file", $timestamp, $exclude, $info_files);
        }
        else {
          $mtime = filemtime("$dir/$file");
          $timestamp = ($mtime > $timestamp) ? $mtime : $timestamp;
          if (preg_match('/^.+\.info$/', $file)) {
            $info_files[] = "$dir/$file";
          }
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
    drupal_exec("$msgcat $dir/general.pot $dir/[^g]*.pot | $msgattrib --no-fuzzy -o $dir/$dir.pot");
    $line = exec("$msgfmt --statistics $dir/$dir.pot 2>&1");
    $words = preg_split('[\s]', $line, -1, PREG_SPLIT_NO_EMPTY);
    $number_of_strings[$version] = $words[3];
    @unlink("$dir/$dir.pot");
  }
  return $number_of_strings[$version];
}
