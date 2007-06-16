#!/usr/bin/php
<?php

// $Id: project-release-create-history.php,v 1.2 2007/06/16 18:38:32 dww Exp $
// $Name:  $

/**
 * @file
 * Dumps out a complete history of releases for a given project.  Each
 * file is an XML representation of all relevant release data for
 * update status checking, security notification, etc.  Every project
 * creates a separate file for the releases for each unique API
 * compatibility term (e.g. for Drupal: 5.x, 6.x, etc).
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 *
 */

// ------------------------------------------------------------
// Required customization
// ------------------------------------------------------------

// The root of your Drupal installation, so we can properly bootstrap
// Drupal. This should be the full path to the directory that holds
// your index.php file, the "includes" subdirectory, etc.
define('DRUPAL_ROOT', '');

// The name of your site. Required so that when we bootstrap Drupal in
// this script, we find the right settings.php file in your sites folder. 
define('SITE_NAME', '');


// ------------------------------------------------------------
// Initialization
// (Real work begins here, nothing else to customize)
// ------------------------------------------------------------

// Check if all required variables are defined
$vars = array(
  'DRUPAL_ROOT' => DRUPAL_ROOT,
  'SITE_NAME' => SITE_NAME,
);
$fatal_err = FALSE;
foreach ($vars as $name => $val) {
  if (empty($val)) {
    print "ERROR: \"$name\" constant not defined, aborting\n";
    $fatal_err = TRUE;
  }
}
if ($fatal_err) {
  exit(1);
}

$script_name = $argv[0];

// Setup variables for Drupal bootstrap
$_SERVER['HTTP_HOST'] = SITE_NAME;
$_SERVER['REQUEST_URI'] = '/' . $script_name;
$_SERVER['SCRIPT_NAME'] = '/' . $script_name;
$_SERVER['PHP_SELF'] = '/' . $script_name;
$_SERVER['SCRIPT_FILENAME'] = $_SERVER['PWD'] .'/'. $script_name;
$_SERVER['PATH_TRANSLATED'] = $_SERVER['SCRIPT_FILENAME'];

if (!chdir(DRUPAL_ROOT)) {
  print "ERROR: Can't chdir(DRUPAL_ROOT), aborting.\n";
  exit(1);
}
// Make sure our umask is sane for generating directories and files.
umask(022);

require_once 'includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

define('BASE_DIRECTORY', DRUPAL_ROOT .'/'. file_create_path(variable_get('project_release_history_directory', 'release-history')));

if (!is_dir(BASE_DIRECTORY)) {
  wd_err(t("ERROR: History directory (%directory) does not exist, aborting.\n", array('%directory' => BASE_DIRECTORY)));
  exit(1);
}

/// @todo Add command-line args to only generate a given project/version.
project_release_history_generate_all();


// ------------------------------------------------------------
// Functions: main work
// ------------------------------------------------------------

/**
 * Figure out what project and API terms to generate the history for.
 */
function project_release_history_generate_all() {
  $api_terms = project_release_compatibility_list();
  wd_msg(t('Generating XML release history files for all projects.'));

  $query = db_query("SELECT DISTINCT(prn.pid), tn.tid FROM {project_release_nodes} prn INNER JOIN {term_node} tn ON prn.nid = tn.nid WHERE tn.tid IN (%s)", implode(',', array_keys($api_terms)));
  $i = 0;
  while ($project = db_fetch_object($query)) {
    project_release_history_generate_project_xml($project->pid, $project->tid);
    $i++;
  }
  wd_msg(t('Completed XML release history files for @num_projects.', array('@num_projects' => format_plural($i, '1 project/version pair', '@count project/version pairs'))));
}

/**
 * Generate the XML history file for a given project name and API
 * compatibility term.
 *
 * @todo If a history file already exists for this combination, this
 * function will generate a new history and atomically replace the old
 * one (currently, just logs to watchdog for debugging).
 *
 * @todo If there's no subdirectory in the directory tree for this
 * project yet, this function creates one.
 *
 * @param $project_nid
 *  Project ID (node id of the project node) to generate history for.
 * @param $api_tid
 *  Taxonomy id (tid) of the API compatibility term to use.
 */
function project_release_history_generate_project_xml($project_nid, $api_tid) {
  $api_vid = _project_release_get_api_vid();
  $api_terms = project_release_compatibility_list();
  if (!isset($api_terms[$api_tid])) {
    wd_err(t('API compatibility term %tid not found.', array('%tid' => $api_tid)));
    return FALSE;
  }
  $api_version = $api_terms[$api_tid];

  // Get project-wide data:
  $sql = "SELECT n.title, n.nid, p.uri, prdv.major FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid LEFT JOIN {project_release_default_versions} prdv ON prdv.nid = n.nid AND prdv.tid = %d WHERE p.nid = %d";
  $query = db_query($sql, $api_tid, $project_nid);
  if (!db_num_rows($query)) {
    wd_err(t('Project ID %pid not found', array('%pid' => $project_nid)));
    return FALSE;
  }
  $project = db_fetch_object($query);
  if (!isset($project->major)) {
    wd_err(t('No release found for project %project_name compatible with %api_version.', array('%project_name' => $project->uri, '%api_version' => $api_version)));
    return FALSE;
  }

  $xml = "<project>\n";
  $xml .= '<title>'. check_plain($project->title) ."</title>\n";
  $xml .= '<short_name>'. check_plain($project->uri) ."</short_name>\n";
  $xml .= '<link>'. url("node/$project->nid", NULL, NULL, TRUE) ."</link>\n";
  $xml .= '<api_version>'. check_plain($api_version) ."</api_version>\n";
  $xml .= "<default_major>$project->major</default_major>\n";
  $xml .= "<releases>\n";

  // Now, build the query for all the releases for this project and term.
  $joins[] = "INNER JOIN {project_release_nodes} prn ON n.nid = prn.nid";
  $where[] = "prn.pid = '%d'";
  $parameters[] = $project->nid;

  // Restrict releases to the specified API version.
  $joins[] = "INNER JOIN {term_node} tn ON tn.nid = prn.nid";
  $where[] = 'tn.tid = %d';
  $parameters[] = $api_tid;

  $query = "SELECT prn.nid, prn.file_path, prn.file_date, prn.file_hash, prn.version, prn.version_major, prn.version_minor, prn.version_patch, prn.version_extra, prn.tag, n.title, n.status FROM {node} n ";

  $query .= implode(' ', $joins);
  $query .= " WHERE " . implode(' AND ', $where);
  $query .= " ORDER BY prn.version_major DESC, prn.version_minor DESC, prn.version_patch DESC, prn.file_date DESC";

  $result = db_query($query, $parameters);
  while ($release = db_fetch_object($result)) {
    $xml .= " <release>\n";
    $xml .= '  <name>'. check_plain($release->title) ."</name>\n";
    $xml .= '  <version>'. check_plain($release->version) ."</version>\n";
    $xml .= '  <tag>'. check_plain($release->tag) ."</tag>\n";
    foreach (array('major', 'minor', 'patch', 'extra') as $type) {
      $vers_type = "version_$type";
      if (isset($release->$vers_type)) {
        $xml .= "  <$vers_type>". check_plain($release->$vers_type) ."</$vers_type>\n";
      }
    }
    if ($release->status) {
      // Published,  so we should include the links.
      $xml .= "  <status>published</status>\n";
      $xml .= '  <release_link>'. url("node/$release->nid", NULL, NULL, TRUE) ."</release_link>\n";
      if (!empty($release->file_path)) {
        $download_link = project_release_download_link($release->file_path, NULL, TRUE);
        $xml .= '  <download_link>'. $download_link['href'] ."</download_link>\n";
      }
    }
    else {
      $xml .= "  <status>unpublished</status>\n";
    }
    // We want to include the rest of these regardless of the status.
    if (!empty($release->file_date)) {
      $xml .= '  <date>'. check_plain($release->file_date) ."</date>\n";
    }
    if (!empty($release->file_hash)) {
      $xml .= '  <mdhash>'. check_plain($release->file_hash) ."</mdhash>\n";
    }
    $term_query = db_query("SELECT v.name AS vocab_name, v.vid, td.name AS term_name, td.tid FROM {term_node} tn INNER JOIN {term_data} td ON tn.tid = td.tid INNER JOIN {vocabulary} v ON td.vid = v.vid WHERE tn.nid = %d AND v.vid != %d", $release->nid, $api_vid);
    $xml_terms = '';
    while ($term = db_fetch_object($term_query)) {
      $xml_terms .= '   <term><name>'. check_plain($term->vocab_name) .'</name>';
      $xml_terms .= '<value>'. check_plain($term->term_name) ."</value></term>\n";
    }      
    if (!empty($xml_terms)) {
      $xml .= "  <terms>\n". $xml_terms ."  </terms>\n";
    }
    $xml .= " </release>\n";
  }
  $xml .= "</releases>\n</project>\n";
  project_release_history_write_xml($project, $api_version, $xml);
}


/**
 * Write out the XML history for a given project and version to a file.
 *
 * @param $project
 *  An object containing (at least) the title and uri of project.
 * @param $api_version
 *  The API compatibility version the history is for.
 * @param $xml
 *  String containing the XML representation of the history.
 */
function project_release_history_write_xml($project, $api_version, $xml) {

  // Setup the filenames we'll be using.  Normally, we'd have to be
  // extra careful with $project->uri to avoid malice here, however,
  // that's validated on the project edit form to prevent any funny
  // characters, so that much is safe.  The rest of these paths are
  // just from the global variables at the top of this script, so we
  // can trust those.  The only one we should be careful of is the
  // taxonomy term for the API compatibility.
  $safe_api_vers = strtr($api_version, '/', '_');
  $project_dir = BASE_DIRECTORY .'/'. $project->uri;
  $project_id = $project->uri .'-'. $safe_api_vers .'.xml';
  $filename = $project_dir .'/'. $project_id;
  $tmp_filename = $filename .'.new';

  // Make sure we've got the right project-specific subdirectory.
  if (!is_dir($project_dir) && !mkdir($project_dir)) {
    wd_err(t("ERROR: mkdir(@dir) failed, can't write history for %project.", array('@dir' => $project_dir, '%project' => $project->title)));
    return FALSE;
  }
  // Make sure the "[project]-[version].xml.new" file doesn't exist.
  if (is_file($tmp_filename) && !unlink($tmp_filename)) {
    wd_err(t("ERROR: unlink(@file) failed, can't write history for %project.", array('@file' => $tmp_filename, '%project' => $project->title)));
    return FALSE;
  }
  // Write the XML history to "[project]-[version].xml.new".
  if (!$hist_fd = fopen($tmp_filename, 'xb')) {
    wd_err(t("ERROR: fopen(@file, 'xb') failed", array('@file' => $file)));
    return FALSE;
  }
  if (!fwrite($hist_fd, $xml)) { 
    wd_err(t("ERROR: fwrite(@file) failed", array('@file' => $tmp_filename)) . '<pre>' . check_plain($xml));
    return FALSE;
  }
  // We have to close this handle before we can rename().
  fclose($hist_fd);

  // Now we can atomically rename the .new into place in the "live" spot.
  if (!rename($tmp_filename, $filename)) {
    wd_err(t("ERROR: rename(@old, $new) failed, can't write history for %project.", array('@old' => $tmp_filename, '@new' => $filename, '%project' => $project->title)));
    return FALSE;
  }
  return TRUE;
}


// ------------------------------------------------------------
// Functions: utility methods
// ------------------------------------------------------------

/**
 * Wrapper function for watchdog() to log notice messages.
 */
function wd_msg($msg, $link = NULL) {
  watchdog('release_history', $msg, WATCHDOG_NOTICE, $link);
}

/**
 * Wrapper function for watchdog() to log error messages.
 */
function wd_err($msg, $link = NULL) {
  watchdog('release_hist_error', $msg, WATCHDOG_ERROR, $link);
}
