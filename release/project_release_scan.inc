<?php
// $Id: project_release_scan.inc,v 1.1.2.3 2007/08/22 16:39:54 thehunmonkgroup Exp $
// $Name:  $

/**
 * All of the release scanning code is probably dead code at this
 * point. It might need to be re-implemented using new system, but we
 * need to be sure we understand what this is doing, how and why.  On
 * drupal.org, we're not going to periodically scan a directory, we're
 * going to generate releases based on CVS tags. We'll have to
 * research if anyone's still using this behavior who wants it to
 * somehow be ported to the new system.
 */

/**
 * This is not a real hook. Just saving this code since it really
 * belongs in project_release_menu() if we ever want it to happen again.
 */
function project_release_scan_menu() {
  if ($may_cache) {
    if (variable_get('project_release_directory', '')) {
      $access = user_access('administer projects');
      $items[] = array('path' => 'admin/settings/project/reload', 'title' => t('scan for releases'), 'callback' => 'project_release_scan', 'access' => $access, 'type' => MENU_NORMAL_ITEM);
    }
  }
}

/**
 * This is not a real hook. Just saving this code since it really
 * belongs in project_release_cron() if we ever want it to happen again.
 */
function project_release_scan_cron() {
  if (variable_get('project_release_directory', '') && time() - variable_get('project_release_directory_last', 0) > variable_get('project_release_directory_interval', 6 * 60 * 60)) {
    variable_set('project_release_directory_last', time());
    // TODO: should be able to optimize this to use less file system calls.
    project_release_scan_directory();
  }
}

function project_release_scan() {
  project_release_scan_directory();
  drupal_set_title(t('Release directory scan'));
  foreach (project_release_scan_directory_results() as $type => $count) {
    drupal_set_message(t('Releases %type: %count.', array('%type' => $type, '%count' => $count)));
  }
  return t('Scan completed.');
}

function project_release_scan_directory($project = NULL) {
  if ($dir = file_create_path(variable_get('project_release_directory', ''))) {
    if ($project) {
      $regexp = "($project)" .'-(.+)\.(tar.gz|zip)';
    }
    else {
      $regexp = '(.+)-(.+)\.(tar.gz|zip)';
    }

    file_scan_directory($dir, $regexp, array('.', '..', 'CVS'), 'project_release_scan_parse');

    // If any releases were previously present but are no longer, unpublish them.
    foreach (project_release_scan_parse() as $project) {
      if ($project->releases) {
        foreach ($project->releases as $rid) {
          project_release_delete($rid);
          project_release_scan_directory_results(t('unpublished'));
        }
      }
    }
  }
  project_releases_list(TRUE);
}

function project_release_scan_directory_results($type = NULL) {
  static $results;
  if ($type) {
    if (!$results[$type]) {
      $results[$type] = 1;
    }
    else {
      $results[$type]++;
    }
  }
  return $results;
}

function project_release_scan_parse($path = NULL) {
  static $projects = array();

  if (is_null($path)) {
    return $projects;
  }
  else {
    $dir = dirname($path);
    $file = basename($path);
    //preg_match('/^(.+?)-([0-9.]+(?:-.*)|[^-]+)\.(tar\.gz|zip)$/', $file, $matches);
    preg_match('/^(.+?)-([0-9.]+(?:-.*)|[^-]+)\.(tar.gz$|zip$)/', $file, $matches);
    list($filename, $name, $version) = $matches;

    // If the project was not previously loaded, load its data, including previous releases.
    if (!$projects[$name]) {
      if ($project = db_fetch_object(db_query(db_rewrite_sql("SELECT n.nid, n.type, n.moderate FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid WHERE p.uri = '%s'"), $name))) {
        if ($releases = project_release_load($project, 0)) {
          // Releases are loaded in an array where the key is the rid and the value the version.
          // We flip the array to more readily test for releases.
          $project->releases = array_flip($releases);
        }
        // Unmoderate nodes that have gotten releases
        if (variable_get('project_release_unmoderate', 0) && $project->moderate) {
          db_query("UPDATE {node} SET moderate = 0, status = 1 WHERE nid = %d", $project->nid);
        }
      }
      else {
        // No project found for this id.
        return;
      }
      $projects[$name] = & $project;
    }
    else {
      $project = & $projects[$name];
    }

    $release->scan = 1;
    $release->nid = $project->nid;
    // If the current version was previously released, add its rid value, so that
    // it will be updated rather than inserted by project_release_save();
    if (is_array($project->releases) && array_key_exists($version, $project->releases)) {
      $release->rid = $project->releases[$version];
      project_release_scan_directory_results(t('updated'));
    }
    else {
      project_release_scan_directory_results(t('created'));
    }
    $release->version = $version;
    $release->path = $path;
    project_release_save($release);
    // This release version has been resaved, so take it out of the array.
    // That way, any releases no longer present can be unpublished in project_release_scan_directory().
    if ($project->releases[$version]) {
      unset($project->releases[$version]);
    }
  }
}


