<?php

/**
 * This update script must be run before a site is updated
 * to project.module CVS if the site is changing from a version
 * prior to 2006-01-05 to one after that date.
 *
 * The script creates a new 'hash' field in project_releases, and
 * sets an initial value for the field.  Subsequent cron runs
 * will check if hash has changed, and if so will reset the
 * changed date of the release.  We need to have an initial
 * value in place so that all releases are not set with a release
 * date of the first cron run.
 *
 * To avoid errors, this update script should be run after
 * the update script update_project_versions.php.
 */

$update_output;
include_once './includes/bootstrap.inc';
drupal_maintenance_theme();
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

drupal_set_title(t('Set project release hashes'));
switch ($GLOBALS['db_type']) {
  case 'pgsql':
    db_add_column($ret, 'created', 'hash', 'varchar(32)', array('not null' => TRUE, 'default' => ''));
    break;
  case 'mysql':
  case 'mysqli':
    db_query("ALTER TABLE project_releases ADD hash varchar(32) NOT NULL default ''");
    break;
}

project_update_release_date();
print theme('maintenance_page', $update_output);

function project_update_release_date() {
  global $update_output;
  project_update_release_date_scan();
  $update_output .= '<p>' . t('%count release hashes updated.', array('%count' => project_update_release_date_result() - 1)) . '</p>';
}

function project_update_release_date_result($type = NULL) {
  static $results = 0;
  $results++;
  return $results;
}

function project_update_release_date_scan() {
  if ($dir = file_create_path(variable_get('project_release_directory', ''))) {
    $regexp = '(.+)-(.+)\.(tar.gz|zip)';
    file_scan_directory($dir, $regexp, array('.', '..', 'CVS'), 'project_update_release_date_parse');
  }
}

// This is an adapted version of project_release_scan_parse().
// Here, we only need to update the hash value.
function project_update_release_date_parse($path = NULL) {
  static $projects = array();

  $dir = dirname($path);
  $file = basename($path);
  preg_match('/^(.+?)-([0-9.]+(?:-.*)|[^-]+)\.(tar.gz$|zip$)/', $file, $matches);
  list($filename, $name, $version) = $matches;
  // If the project was not previously loaded, load its data, including previous releases.
  if (!$projects[$name]) {
    $project = db_fetch_object(db_query(db_rewrite_sql("SELECT n.nid FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid WHERE p.uri = '%s'"), $name));
    if (!$project) {
      // No project found for this id.
      return;
    }
    $projects[$name] = & $project;
  }
  else {
    $project = & $projects[$name];
  }
  $hash = md5_file($path);
  db_query('UPDATE {project_releases} SET hash = "%s" WHERE nid = %d AND version = "%s" AND hash = ""', $hash, $project->nid, $version);
  project_update_release_date_result();

}

/**
 * From update.php
 */
function db_add_column(&$ret, $table, $column, $type, $attributes = array()) {
  if (array_key_exists('not null', $attributes) and $attributes['not null']) {
    $not_null = 'NOT NULL';
  }
  if (array_key_exists('default', $attributes)) {
    if (is_null($attributes['default'])) {
      $default_val = 'NULL';
      $default = 'default NULL';
    }
    elseif ($attributes['default'] === FALSE) {
      $default = '';
    }
    else {
      $default_val = "$attributes[default]";
      $default = "default $attributes[default]";
    }
  }

  $ret[] = update_sql("ALTER TABLE {". $table ."} ADD $column $type");
  if ($default) { $ret[] = update_sql("ALTER TABLE {". $table ."} ALTER $column SET $default"); }
  if ($not_null) {
    if ($default) { $ret[] = update_sql("UPDATE {". $table ."} SET $column = $default_val"); }
    $ret[] = update_sql("ALTER TABLE {". $table ."} ALTER $column SET NOT NULL");
  }
}
?>
