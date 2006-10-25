<?php
// $Id: project_release_update.php,v 1.1.2.1 2006/10/25 00:28:44 dww Exp $

/**
 * @file
 * Converts data from the old {project_releases} table into
 * project_release nodes.
 *
 * TODO:
 * - resolve the whole -1 vs. NULL thing for version fields
 * - figure out if we're going to save as separate fields, anyway
 * - anywhere else you see "TODO" in this file. ;)
 *
 */

/**
 * Generates the appropriate tag name for a core release.
 * BEWARE: This is drupal.org specific code.
 */
function generate_core_tag($node) {
  $tag = 'DRUPAL-';
  $tag .= $node->version_major . '-';
  $tag .= $node->version_minor . '-';
  $tag .= $node->version_patch;
  if (!empty($node->version_extra)) {
    $tag .= '-' . strtoupper(preg_replace('/(.+)(\d+)/', '\1-\2', $node->version_extra));
  }
  return $tag;
} 

function convert_all() {
  $releases = db_query("SELECT pr.*, n.uid, n.title AS project_title FROM {project_releases} pr INNER JOIN {node} n ON pr.nid = n.nid");
  while ($old_release = db_fetch_object($releases)) {
    convert_one($old_release);
  }
}

/**
 * Determines if a given entry in the {project_releases} table
 * corresponds to a real release, or a nightly development snapshot,
 * and converts it into the appropriate kind of release node.
 *
 * BEWARE: This is drupal.org specific code. In our case, the core
 * drupal system (project nid #3060) is the only project that's ever
 * had "real" releases so far. Everything else has been a nightly dev
 * snapshot release. If your site has a different usage, please modify
 * the logic in here to meet your needs.
 *
 */
function convert_one($old_release) {
  // First, save everything that's shared, regardless of the version/type

  // Things that go in {node} or {node_revisions}
  $node->type = 'project_release';
  $node->uid = $old_release->uid ? $old_release->uid : 1;
  $node->created = $old_release->created;
  $node->changed = $old_release->changed;
  $node->body = $old_release->changes ? $old_release->changes : '';
  $node->teaser = node_teaser($node->body);
  $node->filter = variable_get('filter_default_format', 1);
  $node->status = $old_release->status;
  $node->revision = 1;
  $node->promote = 0;
  $node->comment = 0;

  // Things that go in {project_release_nodes} (that don't depend on version)
  $node->pid = $old_release->nid;
  $node->file_path = $old_release->path;
  $node->file_date = $old_release->changed;
  $node->file_hash = $old_release->hash;

  // Now, depending on the project and version, fill in the rest.
  if ($old_release->nid == 3060) {
//    $node->version_super_major = -1;
//    $node->version_super_minor = -1;
    if ($old_release->version == 'cvs') {
      $node->version_major = 5;
      $node->version_minor = 0;
      $node->version_patch = 0;
      $node->version_extra = 'dev';
      $node->rebuild = 1;
      $node->tag = 'TRUNK';
    }
    else {
      preg_match('/(\d+)\.(\d+)\.(\d+)(-?)(.+)?/', $old_release->version, $matches);
      $node->version_major = $matches[1];
      $node->version_minor = $matches[2];
      $node->version_patch = $matches[3];
      $node->version_extra = $matches[5] ? $matches[5] : NULL;
      $node->tag = generate_core_tag($node);
      $node->rebuild = 0;
    }
  }
  elseif ($old_release->version == 'cvs') {
    // The "cvs" version is a nightly tarball from the trunk
//    $node->version_super_major = -1;
//    $node->version_super_minor = -1;
    $node->version_major = 0;
    $node->version_minor = 0;
//    $node->version_patch = -1;
    $node->version_extra = 'dev';
    $node->tag = 'TRUNK';
    $node->rebuild = 1;
  }
  else {
    // Nightly tarball from a specific branch.
    preg_match('/(\d+)\.(\d+)\.(\d+)/', $old_release->version, $matches);
    if ($matches[3] != 0) {
      dprint_r("warning: release $old_release->rid of $old_release->project_title has unexpected patch-level version ($matches[3])");
    }
    $node->version_super_major = $matches[1];
    $node->version_super_minor = $matches[2];
    $node->version_major = 1;
    $node->version_minor = 0;
//    $node->version_patch = -1;
    $node->version_extra = 'dev';
    $node->tag = 'DRUPAL-' . $matches[1] . '-' . $matches[2];
    $node->rebuild = 1;
  }

  // Now, set the right kind of title.
  $version = '';
  if ($node->tag == 'TRUNK' && !isset($node->version_patch)) {
    $version = t('TRUNK');
  }
  else {
    $version = theme('project_release_version', $node);
  }
  $node->title = t('%project %version', array('%project' => $old_release->project_title, '%version' => $version));
  if ($node->rebuild) {
    $node->title .= ' (' . t('nightly development snapshot') . ')';
  }

  // Now, we can actually create the node.
  node_save($node);

  // Next, update all the issues with the old revision id (rid) to
  // point to the new nid, instead.
  $nid = $node->nid;
  $rid = $old_release->rid;
  db_query("UPDATE {project_issues} SET rid = %d WHERE rid = %d", $nid, $rid);
  // TODO: {project_comments} !!!
  // {project_comments} is much harder, since there's serialized data. :( 

  // TODO: update {cvs_tags} table to put in a value for the "release
  // nid" column so we know this project nid/tag combo has a release?
}

include_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

if (!module_exist('project_release')) {
  print '<b>' . t('ERROR: project_release_update.php requires that you first install the project_release.module') . '</b>';
  exit(1);
}

convert_all();

// TODO: user feedback, progress, etc.
// TODO: drop old {project_releases} table once we're convinced it worked
