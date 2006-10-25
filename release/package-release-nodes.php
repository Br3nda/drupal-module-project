#!/usr/local/bin/php
<?php

// $Id: package-release-nodes.php,v 1.1.2.1 2006/10/25 09:27:36 dww Exp $

/**
 * @file
 * Automated packaging script to generate tarballs from release nodes.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 *
 * TODO:
 * - nightly rebuilds from branches (checking timestamps, etc)
 * - better error propagation and robustness
 * - translation stats
 * - decide if this is going to run via hook_cron(), cron, and/or
 *   on-demand directly from with project_release.module 
 * 
 */

// Settings
$drupal_root = '/Users/wright/drupal/cvs/4.7/core.do';
$dest_root = $drupal_root;
$dest_rel = 'files/projects';
$license = '/Users/wright/drupal/release-package/contributions/LICENSE.txt';
$tmp_dir = '/tmp/drupal';
$tar = '/usr/bin/tar';

//putenv('CVSROOT=:pserver:anonymous@cvs.drupal.org:/cvs/drupal');
putenv('CVSROOT=/cvs/drupal');

// Find what site we're running under
if ($argv[1]) {
  $site_name = $argv[1];
}
else {
//  $site_name = 'drupal.org';
  $site_name = 'iskra.local';
}
$script_name = 'package-release-nodes.php';

$_SERVER['HTTP_HOST'] = $site_name;
$_SERVER['REQUEST_URI'] = '/' . $script_name;
$_SERVER['SCRIPT_NAME'] = '/' . $script_name;
$_SERVER['PHP_SELF'] = '/' . $script_name;
$_SERVER['SCRIPT_FILENAME'] = $_SERVER['PWD'] . '/' . $script_name;
$_SERVER['PATH_TRANSLATED'] = $_SERVER['SCRIPT_FILENAME'];

chdir($drupal_root);
require_once 'includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

global $repositories;
initialize_repository_info();
watchdog('package_debug', '<pre>' . var_export($repositories, TRUE));

package_release_tags();


// ------------------------------------------------------------ 
// Functions: main methods
// ------------------------------------------------------------ 

function package_release_tags() {
  watchdog('release_package', "Starting to package all releases from tags");
  $query = db_query("SELECT pp.uri, prn.nid, prn.tag, prn.version, c.directory, c.rid FROM {project_release_nodes} prn INNER JOIN {project_projects} pp ON prn.pid = pp.nid INNER JOIN {cvs_projects} c ON prn.pid = c.nid WHERE prn.rebuild = 0 AND (prn.file_path IS NULL OR prn.file_path = '')");
  while ($release = db_fetch_object($query)) {
    $id = $release->uri . '-' . $release->version;
    $tag = $release->tag;
    $nid = $release->nid;
    $rev = $tag == 'TRUNK' ? '' : "-r $tag";
    watchdog('release_package', t('Working on %type release: %id from tag: %tag', array('%type' => $release->rid == 1 ? t('core') : t('contrib'), '%id' => theme_placeholder($id), '%tag' => theme_placeholder($tag))));
    if ($release->rid == 1) {
      package_release_core_from_tag($nid, $id, $rev);
    }
    else {
      package_release_contrib_from_tag($nid, $id, $rev, $release->directory);
    }
  }
  watchdog('release_package', "Done packaging all tags");
}

function package_release_core_from_tag($nid, $id, $rev) {
  global $tmp_dir, $tar, $repositories, $dest_root, $dest_rel;
  $rid = 1;

  if (!chdir($tmp_dir)) {
    watchdog('release_package', t("ERROR: Can't chdir(%dir)", array('%dir' => $tmp_dir)));
    return;
  }

  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  // Actually generate the tarball:
  `cvs -q co $rev -Pd $id drupal`;
  `$tar -c --exclude=CVS --file=- $id | gzip -9 --no-name > $full_dest`;
  `rm -rf $id`;
  // TODO: need better error checking

  package_release_update_node($nid, $file_path);
}

function package_release_contrib_from_tag($nid, $id, $rev, $directory) {
  global $tmp_dir, $tar, $license, $repositories, $dest_root, $dest_rel;
  $rid = 2;

  $parts = split('/', $directory);
  # modules, themes or theme-engines
  $type = $parts[1];
  # specific directory (same as uri)
  $localdir = $parts[2];

  $basedir = $repositories[$rid]['modules'] . '/' . $type;
  $fulldir = $basedir . '/' . $localdir;
  $file_name = $id . '.tar.gz';
  $file_path = $dest_rel . '/' . $file_name;
  $full_dest = $dest_root . '/' . $file_path;

  if (!chdir($tmp_dir)) {
    watchdog('release_package', t("ERROR: Can't chdir(%dir)", array('%dir' => $tmp_dir)));
    return;
  }

  // Actually generate the tarball:
  `cvs -q co $rev $fulldir`;
  chdir($basedir);
  // Link not copy, since we want to preserve the date...
  `ln -sf $license $localdir/LICENSE.txt`;
  // 'h' is for dereference, we want to include the license file, not the link
  `$tar -ch --exclude=CVS --file=- $localdir | gzip -9 --no-name > $full_dest`;
#  `rm -rf $localdir`;
  wprint("rm -rf $localdir");  
  // TODO: need better error checking

  package_release_update_node($nid, $file_path);
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

function package_release_branches() {
  watchdog('release_package', "Starting to package all releases from branches");
  // TODO
  watchdog('release_package', "Done packaging all branches");
}


// ------------------------------------------------------------ 
// Functions: utility methods
// ------------------------------------------------------------ 

function wprint($var) {
  watchdog('package_debug', '<pre>' . var_export($var, TRUE));
}

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

