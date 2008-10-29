#!/usr/bin/php
<?php

// $Id: project-release-create-history.php,v 1.10.2.5 2008/10/29 00:19:52 dww Exp $

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
project_list_generate();

// ------------------------------------------------------------
// Functions: main work
// ------------------------------------------------------------

/**
 * Figure out what project and API terms to generate the history for.
 */
function project_release_history_generate_all() {
  $api_terms = project_release_compatibility_list();
  wd_msg(t('Generating XML release history files for all projects.'));

  // Generate all.xml files for projects with releases.
  $query = db_query("SELECT DISTINCT(pid) FROM {project_release_nodes}");
  $i = 0;
  while ($project = db_fetch_object($query)) {
    project_release_history_generate_project_xml($project->pid);
    $i++;
  }
  wd_msg(format_plural($i, 'Generated an XML release history summary for a project.', 'Generated XML release history summaries for @count projects.'));

  // Generate XML files based on API compatibility.
  $tids = array_keys($api_terms);
  $placeholders = implode(',', array_fill(0, count($tids), '%d'));
  $query = db_query("SELECT DISTINCT(prn.pid), tn.tid FROM {project_release_nodes} prn INNER JOIN {term_node} tn ON prn.nid = tn.nid WHERE tn.tid IN ($placeholders)", $tids);
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
 *   Project ID (node id of the project node) to generate history for.
 * @param $api_tid
 *   Taxonomy id (tid) of the API compatibility term to use, or NULL if
 *   all terms are considered.
 */
function project_release_history_generate_project_xml($project_nid, $api_tid = NULL) {
  $api_vid = _project_release_get_api_vid();

  if (isset($api_tid)) {
    // Restrict output to a specific API compatibility term.
    $api_terms = project_release_compatibility_list();
    if (!isset($api_terms[$api_tid])) {
      wd_err(t('API compatibility term %tid not found.', array('%tid' => $api_tid)));
      return FALSE;
    }
    $api_version = $api_terms[$api_tid];

    // Get project-wide data:
    $sql = "SELECT DISTINCT n.title, n.nid, n.status, p.uri, u.name AS username FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid INNER JOIN {project_release_supported_versions} prsv ON prsv.nid = n.nid INNER JOIN {users} u ON n.uid = u.uid WHERE prsv.tid = %d AND p.nid = %d";
    $query = db_query($sql, $api_tid, $project_nid);
  }
  else {
    // Consider all API compatibility terms.
    $api_version = 'all';
    $sql = "SELECT n.title, n.nid, n.status, p.uri, u.name AS username FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid INNER JOIN {users} u ON n.uid = u.uid WHERE p.nid = %d";
    $query = db_query($sql, $project_nid);
  }
  if (!db_num_rows($query)) {
    wd_err(t('Project ID %pid not found', array('%pid' => $project_nid)));
    return FALSE;
  }
  $project = db_fetch_object($query);

  $xml = '<title>'. check_plain($project->title) ."</title>\n";
  $xml .= '<short_name>'. check_plain($project->uri) ."</short_name>\n";
  $xml .= '<dc:creator>'. check_plain($project->username) ."</dc:creator>\n";
  $xml .= '<api_version>'. check_plain($api_version) ."</api_version>\n";
  if (!$project->status) {
    // If it's not published, we can skip the rest of this and bail.
    $xml .= "<project_status>unpublished</project_status>\n";
    project_release_history_write_xml($xml, $project, $api_version);
    return;
  }

  $project_status = 'published';
  if (isset($api_tid)) {
    // Include the info about supported and recommended major versions.
    $query = db_query("SELECT major, supported, recommended FROM {project_release_supported_versions} WHERE nid = %d AND tid = %d", $project_nid, $api_tid);
    $supported_majors = array();
    while ($version_info = db_fetch_object($query)) {
      if ($version_info->supported) {
        $supported_majors[] = $version_info->major;
      }
      if ($version_info->recommended) {
        $recommended_major = $version_info->major;
      }
    }
    if (isset($recommended_major)) {
      $xml .= '<recommended_major>'. $recommended_major ."</recommended_major>\n";
    }
    if (empty($supported_majors)) {
      $project_status = 'unsupported';
    }
    else {
      $xml .= '<supported_majors>'. implode(',', $supported_majors) ."</supported_majors>\n";
      // To avoid confusing existing clients, include <default_major>, too.
      $xml .= '<default_major>'. min($supported_majors) ."</default_major>\n";
    }
  }

  $xml .= '<project_status>'. $project_status ."</project_status>\n";
  $xml .= '<link>'. url("node/$project->nid", NULL, NULL, TRUE) ."</link>\n";

  // To prevent the update(_status) module from having problems parsing the XML,
  // the terms need to be at the end of the information for the project.
  $term_query = db_query("SELECT v.name AS vocab_name, v.vid, td.name AS term_name, td.tid FROM {term_node} tn INNER JOIN {term_data} td ON tn.tid = td.tid INNER JOIN {vocabulary} v ON td.vid = v.vid WHERE tn.nid = %d", $project->nid);
  $xml_terms = '';
  while ($term = db_fetch_object($term_query)) {
    $xml_terms .= '   <term><name>'. check_plain($term->vocab_name) .'</name>';
    $xml_terms .= '<value>'. check_plain($term->term_name) ."</value></term>\n";
  }
  if (!empty($xml_terms)) {
    $xml .= "  <terms>\n". $xml_terms ."  </terms>\n";
  }

  // Now, build the query for all the releases for this project and term.
  $joins = array();
  $where = array();
  $parameters = array();
  $fields = array(
    'prn.nid',
    'prn.file_path',
    'prn.file_date',
    'prn.file_hash',
    'prn.rebuild',
    'prn.version',
    'prn.version_major',
    'prn.version_minor',
    'prn.version_patch',
    'prn.version_extra',
    'prn.tag',
    'n.title',
    'n.status',
  );

  $joins[] = "INNER JOIN {project_release_nodes} prn ON n.nid = prn.nid";
  $where[] = "prn.pid = '%d'";
  $parameters[] = $project->nid;

  $joins[] = "INNER JOIN {term_node} tn ON tn.nid = prn.nid";
  // Restrict releases to the specified API version.
  if (isset($api_tid)) {
    $where[] = 'tn.tid = %d';
    $parameters[] = $api_tid;
  }
  else {
    // If we're building a list for all versions, then we also need to sort
    // our releases based on the API term's weight.  Also, to prevent
    // duplicate release nodes and ensure we're just looking at the API term,
    // we need to add a WHERE clause for the API vocabulary id.
    $joins[] = "INNER JOIN {term_data} td ON tn.tid = td.tid";
    $where[] = 'td.vid = '. _project_release_get_api_vid();
    $fields[] = 'td.weight';
  }

  $query = "SELECT ". implode(', ', $fields) ." FROM {node} n ";
  $query .= implode(' ', $joins);
  $query .= " WHERE " . implode(' AND ', $where);
  $result = db_query($query, $parameters);

  $releases = array();
  while ($release = db_fetch_object($result)) {
    $releases[] = $release;
  }

  if (empty($releases)) {
    // Nothing more to include for this project, wrap up and return.
    project_release_history_write_xml($xml, $project, $api_version);
    return;
  }

  // Sort the releases based on our custom sorting function.
  usort($releases, "_release_sort");

  $xml .= "<releases>\n";
  foreach ($releases as $release) {
    $xml .= " <release>\n";
    $xml .= '  <name>'. check_plain($release->title) ."</name>\n";
    $xml .= '  <version>'. check_plain($release->version) ."</version>\n";
    if (!empty($release->tag) && $tag = check_plain($release->tag)) {
      $xml .= '  <tag>'. $tag ."</tag>\n";
    }
    foreach (array('major', 'minor', 'patch', 'extra') as $type) {
      $vers_type = "version_$type";
      if (isset($release->$vers_type)) {
        $xml .= "  <$vers_type>". check_plain($release->$vers_type) ."</$vers_type>\n";
      }
    }
    if ($release->status) {
      // Published, so we should include the links.
      $xml .= "  <status>published</status>\n";
      $xml .= '  <release_link>'. url("node/$release->nid", NULL, NULL, TRUE) ."</release_link>\n";
      if (!empty($release->file_path)) {
        $download_link = theme('project_release_download_link', $release->file_path, NULL, TRUE);
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
  $xml .= "</releases>\n";
  project_release_history_write_xml($xml, $project, $api_version);
}


/**
 * Write out the XML history for a given project and version to a file.
 *
 * @param $xml
 *   String containing the XML representation of the history.
 * @param $project
 *   An object containing (at least) the title and uri of project.
 * @param $api_version
 *   The API compatibility version the history is for.
 */
function project_release_history_write_xml($xml, $project = NULL, $api_version = NULL) {

  // Dublin core namespace according to http://dublincore.org/documents/dcmi-namespace/
  $dc_namespace = 'xmlns:dc="http://purl.org/dc/elements/1.1/"';
  if (!isset($project)) {
    // We are outputting a global project list.
    $project_dir = BASE_DIRECTORY .'/project-list';
    $filename = $project_dir .'/project-list-all.xml';
    $tmp_filename = $filename .'.new';
    $errors = array(
      'mkdir'  => t("ERROR: mkdir(@dir) failed, can't write project list.", array('@dir' => $project_dir)),
      'unlink' => t("ERROR: unlink(@file) failed, can't write project list.", array('@file' => $tmp_filename)),
      'rename' => t("ERROR: rename(@old, @new) failed, can't write project list.", array('@old' => $tmp_filename, '@new' => $filename))
    );
    $full_xml = '<projects '. $dc_namespace .">\n". $xml ."</projects>\n";
  }
  else {
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
    $errors = array(
      'mkdir'  => t("ERROR: mkdir(@dir) failed, can't write history for %project.", array('@dir' => $project_dir, '%project' => $project->title)),
      'unlink' => t("ERROR: unlink(@file) failed, can't write history for %project.", array('@file' => $tmp_filename, '%project' => $project->title)),
      'rename' => t("ERROR: rename(@old, @new) failed, can't write history for %project.", array('@old' => $tmp_filename, '@new' => $filename, '%project' => $project->title))
    );
    $full_xml = '<project '. $dc_namespace .">\n". $xml ."</project>\n";
  }

  // Make sure we've got the right project-specific subdirectory.
  if (!is_dir($project_dir) && !mkdir($project_dir)) {
    wd_err($errors['mkdir']);
    return FALSE;
  }
  // Make sure the "[project]-[version].xml.new" file doesn't exist.
  if (is_file($tmp_filename) && !unlink($tmp_filename)) {
    wd_err($errors['unlink']);
    return FALSE;
  }
  // Write the XML history to "[project]-[version].xml.new".
  if (!$hist_fd = fopen($tmp_filename, 'xb')) {
    wd_err(t("ERROR: fopen(@file, 'xb') failed", array('@file' => $tmp_filename)));
    return FALSE;
  }
  if (!fwrite($hist_fd, $full_xml)) {
    wd_err(t("ERROR: fwrite(@file) failed", array('@file' => $tmp_filename)) . '<pre>' . check_plain($full_xml));
    return FALSE;
  }
  // We have to close this handle before we can rename().
  fclose($hist_fd);

  // Now we can atomically rename the .new into place in the "live" spot.
  if (!_rename($tmp_filename, $filename)) {
    wd_err($errors['rename']);
    return FALSE;
  }
  return TRUE;
}

/**
 * Generate a list of all projects available on this server.
 */
function project_list_generate() {
  $api_vid = _project_release_get_api_vid();
  
  $query = db_query("SELECT n.title, n.nid, n.status, p.uri, u.name AS username FROM {node} n INNER JOIN {project_projects} p ON n.nid = p.nid INNER JOIN {users} u ON n.uid = u.uid");
  if (!db_num_rows($query)) {
    wd_err(t('No projects found on this server.'));
    return FALSE;
  }
  $xml = '';
  while ($project = db_fetch_object($query)) {
    $xml .= " <project>\n";
    $xml .= '  <title>'. check_plain($project->title) ."</title>\n";
    $xml .= '  <short_name>'. check_plain($project->uri) ."</short_name>\n";
    $xml .= '  <link>'. url("node/$project->nid", NULL, NULL, TRUE) ."</link>\n";
    $xml .= '  <dc:creator>'. check_plain($project->username). "</dc:creator>\n";
    $term_query = db_query("SELECT v.name AS vocab_name, v.vid, td.name AS term_name, td.tid FROM {term_node} tn INNER JOIN {term_data} td ON tn.tid = td.tid INNER JOIN {vocabulary} v ON td.vid = v.vid WHERE tn.nid = %d", $project->nid);
    $xml_terms = '';
    while ($term = db_fetch_object($term_query)) {
      $xml_terms .= '   <term><name>'. check_plain($term->vocab_name) .'</name>';
      $xml_terms .= '<value>'. check_plain($term->term_name) ."</value></term>\n";
    }
    if (!empty($xml_terms)) {
      $xml .= "  <terms>\n". $xml_terms ."  </terms>\n";
    }
    if (!$project->status) {
      // If it's not published, we can skip the rest for this project.
      $xml .= "  <project_status>unpublished</project_status>\n";
    }
    else {
      $xml .= "  <project_status>published</project_status>\n";
      // Include a list of API terms if available.
      $term_query = db_query("SELECT DISTINCT(td.tid), td.name AS term_name FROM {project_release_nodes} prn INNER JOIN {term_node} tn ON prn.nid = tn.nid INNER JOIN {term_data} td ON tn.tid = td.tid WHERE prn.pid = %d AND td.vid = %d ORDER BY td.weight ASC", $project->nid, $api_vid);
      $xml_api_terms = '';
      while ($api_term = db_fetch_object($term_query)) {
        $xml_api_terms .= '   <api_version>'. check_plain($api_term->term_name) ."</api_version>\n";
      }      
      if (!empty($xml_api_terms)) {
        $xml .= "  <api_versions>\n". $xml_api_terms ."  </api_versions>\n";
      }
    }
    
    $xml .= " </project>\n";
  }
  project_release_history_write_xml($xml);
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
  watchdog('release_hist_err', $msg, WATCHDOG_ERROR, $link);
}

/**
 * Rename on Windows isn't atomic like it is on *nix systems.
 * See http://www.php.net/rename about this bug.
 */
function _rename($oldfile, $newfile) {
  if (substr(PHP_OS, 0, 3) == 'WIN') {
    if (copy($oldfile, $newfile)) {
      unlink($oldfile);
      return TRUE;
    }
    return FALSE;
  }
  else {
    return rename($oldfile, $newfile);
  }
}

/**
 * Sorting function to ensure releases are in the right order in the XML file.
 *
 * Loop over the fields in the release node we care about, and the first field
 * that differs between the two releases determines the order.
 *
 * We first check the 'weight' (of the API version term) for when we're
 * building a single list of all versions, not a per-API version listing. In
 * this case, lower numbers should float to the top.
 *
 * We also need to special-case the 'rebuild' field, which is how we know if
 * it's a dev snapshot or official release. Rebuild == 1 should always come
 * last within a given major version, since that's how update_status expects
 * the ordering to ensure that we never recommend a -dev release if there's an
 * official release available. So, like weight, the lower number for 'rebuild'
 * should float to the top.
 *
 * For every other field, we want the bigger numbers come first.
 *
 * @see project_release_history_generate_project_xml()
 * @see usort()
 */
function _release_sort($a, $b) {
  // This array maps fields in the release node to the sort order, where -1
  // means to sort like Drupal weights, and +1 means the bigger numbers are
  // higher in the listing.
  $fields = array(
    'weight' => -1,
    'version_major' => 1,
    'rebuild' => -1,
    'version_minor' => 1,
    'version_patch' => 1,
    'file_date' => 1,
  );
  foreach ($fields as $field => $sign) {
    if (!isset($a->$field) && !isset($b->$field)) {
      continue;
    }
    if ($a->$field == $b->$field) {
      continue;
    }
    return ($a->$field < $b->$field) ? $sign : (-1 * $sign);
  }
}
