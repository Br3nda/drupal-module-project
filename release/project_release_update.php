<?php
// $Id: project_release_update.php,v 1.7.2.1 2008/10/29 00:19:52 dww Exp $

/**
 * @file
 * Converts data from the old {project_releases} table into
 * project_release nodes.
 */

/**
 * Generates the appropriate tag name for a core release.
 * BEWARE: This is drupal.org specific code.
 */
function generate_core_tag($node) {
  $tag = 'DRUPAL-';
  $tag .= $node->version_major . '-';
  if (isset($node->version_minor)) {
    $tag .= $node->version_minor . '-';
  }
  $tag .= $node->version_patch;
  if (!empty($node->version_extra)) {
    $tag .= '-' . strtoupper(preg_replace('/(.+)(\d+)/', '\1-\2', $node->version_extra));
  }
  return $tag;
}

/**
 * Iterates through the {project_projects} table and adds the
 * appropriate record to the {project_release_projects} table for each
 * entry.
 *
 * BEWARE: This function contains drupal.org-specific code.  Please
 * modify the arrays and setting commented below to suit your own
 * site's needs.
 */
function populate_project_release_projects() {
  list($usec, $sec) = explode(' ', microtime());
  $start = (float)$usec + (float)$sec;

  $num_prp = db_result(db_query("SELECT count(nid) FROM {project_release_projects}"));
  $num_projects = db_result(db_query("SELECT count(nid) FROM {project_projects}"));
  if ($num_prp == $num_projects) {
    print t("The {project_release_projects} table is already full") .'<br />';
    return;
  }
  // First, insert a record with the right nid for all projects
  db_query("DELETE FROM {project_release_projects}");
  db_query("INSERT INTO {project_release_projects} (nid, releases) SELECT nid, 1 FROM {project_projects}");

  // Now, special-cases we need to handle:

  // All the projects that will not want releases enabled
  // BEWARE: drupal.org-specific
  $no_release_projects = array(
    3202 => 'drupal.org maintenance',
    3213 => 'user experience',
    18753 => 'documentation',
    43378 => 'worldpay (ecommerce contrib)',
    67060 => 'event_views (event contrib)',
    67375 => 'location_views (location contrib)',
    75541 => 'inventorymangement (ecommerce contrib)',
  );
  // Any projects with a custom version format string (just core)
  // BEWARE: drupal.org-specific
  $version_formats = array(
    3060 => '!major%minor%patch#extra',
    11093 => '!major%minor%patch#extra',
  );
  // Set the right site-wide default for everything else...
  // BEWARE: drupal.org-specific
  variable_set('project_release_default_version_format', '!api#major%patch#extra');

  foreach ($no_release_projects as $nid => $name) {
    db_query("UPDATE {project_release_projects} SET releases = 0 WHERE nid = %d", $nid);
  }
  foreach ($version_formats as $nid => $format) {
    db_query("UPDATE {project_release_projects} SET version_format = '%s' WHERE nid = %d", $format, $nid);
  }

  $num_prp = db_result(db_query("SELECT count(nid) FROM {project_release_projects}"));
  list($usec, $sec) = explode(' ', microtime());
  $stop = (float)$usec + (float)$sec;
  $diff = round(($stop - $start) * 1000, 2);
  print t('Added %num records to the {project_release_projects} table in %ms ms', array('%num' => $num_prp, '%ms' => $diff)) .'<br />';
}

/**
 * Iterates through the entire {project_releases} table and converts
 * each entry into a new release node.
 */
function convert_all_releases() {
  // First, re-load into memory mappings that we completed on previous runs
  global $nids_by_rid, $api_taxonomy;
  $api_taxonomy = project_release_get_api_taxonomy();

  $query = db_query("SELECT nid, rid FROM {project_release_legacy}");
  while ($result = db_fetch_object($query)) {
    $nids_by_rid[$result->rid] = $result->nid;
    if (module_exists('project_issue')) {
      db_query("UPDATE {project_issues} SET rid = %d WHERE rid = %d", $result->nid, $result->rid);
    }
  }

  // We desperately need an index on rid for the {project_issues}
  // table while doing the conversion, otherwise we spend between
  // 30-60 minutes with the UPDATE to repair the rid.  With this key,
  // it only takes about 1 minute on the whole d.o DB.
  db_query("ALTER TABLE {project_issues} ADD KEY rid (rid)");

  $num_converted = 0;
  $num_considered = 0;
  $start_time = time();
  $releases = db_query("SELECT pr.*, n.uid, n.title AS project_title FROM {project_releases} pr INNER JOIN {node} n ON pr.nid = n.nid LEFT JOIN {project_release_legacy} prl ON pr.rid = prl.rid WHERE prl.rid IS NULL");
  while ($old_release = db_fetch_object($releases)) {
    if (convert_release($old_release)) {
      $num_converted++;
    }
    $num_considered++;
  }

  print t('Considered %num_considered releases, converted %num_converted into nodes in %interval', array('%num_considered' => $num_considered, '%num_converted' => $num_converted, '%interval' => format_interval(time() - $start_time))) .'<br />';
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
 */
function convert_release($old_release) {
  list($usec, $sec) = explode(' ', microtime());
  $start = (float)$usec + (float)$sec;

  global $nids_by_rid, $api_taxonomy;

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
    if ($old_release->version == 'cvs') {
      // TODO: maybe we should just leave this "release" alone,
      $node->version_major = 5;
      $node->version_patch = 'x';
      $node->version_extra = 'dev';
      $node->rebuild = 1;
      $node->tag = 'HEAD';
      $target_api = '5.x';
    }
    elseif (preg_match('/^(\d+)\.(\d+)(-.+)?$/', $old_release->version, $matches)) {
      // Handle "5.x" style releases
      $node->version_major = $matches[1];
      $node->version_patch = $matches[2];
      $node->version_extra = preg_replace('/^-(.+)$/', '\1', $matches[3]);
      $node->tag = generate_core_tag($node);
      $node->rebuild = 0;
      $target_api = "$matches[1].x";
    }
    elseif (preg_match('/^(\d+)\.(\d+)\.(\d+)(-.+)?$/', $old_release->version, $matches)) {
      // Handle "4.7.x" style releases
      $node->version_major = $matches[1];
      $node->version_minor = $matches[2];
      $node->version_patch = $matches[3];
      $node->version_extra = preg_replace('/^-(.+)$/', '\1', $matches[4]);
      $node->tag = generate_core_tag($node);
      $node->rebuild = 0;
      $target_api = "$matches[1].$matches[2].x";
    }
    else {
      print t('<b>ERROR:</b> release %old_release_rid of %old_release_project_title has malformed version (%old_release_version)', array('%old_release_rid' => $old_release->rid, '%old_release_project_title' => $old_release->project_title, '%old_release_version' => $old_release->version)) .'<br />';
      return false;
    }
  }
  elseif ($old_release->version == 'cvs') {
    // The "cvs" version is a nightly tarball from the trunk
    $node->version_extra = 'dev';
    $node->tag = 'HEAD';
    $node->rebuild = 1;
  }
  else {
    // Nightly tarball from a specific branch.
    preg_match('/^(\d+)\.(\d+)\.(\d+)$/', $old_release->version, $matches);
    if ($matches[3] != 0) {
      print t('<b>WARNING:</b> release %old_release_rid of %old_release_project_title has unexpected patch-level version (%matches)', array('%old_release_rid' => $old_release->rid, '%old_release_project_title' => $old_release->project_title, '%matches' => $matches[3])) .'<br />';
    }
    $target_api = "$matches[1].$matches[2].x";
    $node->version_major = 1;
    $node->version_patch = 'x';
    $node->version_extra = 'dev';
    $node->tag = 'DRUPAL-' . $matches[1] . '-' . $matches[2];
    $node->rebuild = 1;
  }

  if (isset($target_api)) {
    foreach ($api_taxonomy as $i => $term) {
      if ($term->name == $target_api) {
        $node->taxonomy[$term->tid] = $term->tid;
        $node->version_api_tid = $term->tid;
        break;
      }
    }
  }

  // Now, set the right kind of title.
  $version = '';
  if ($node->tag == 'HEAD') {
    if ($old_release->nid == 3060) {
      $verson = '5.x-dev';
    }
    else {
      $version = t('HEAD');
    }
  }
  else {
    $version = project_release_get_version($node);
  }
  $node->title = t('%project %version', array('%project' => $old_release->project_title, '%version' => $version));
  $node->version = $version;

  list($usec, $sec) = explode(' ', microtime());
  $pre_save = (float)$usec + (float)$sec;
  // Now, we can actually create the node.
  node_save($node);
  list($usec, $sec) = explode(' ', microtime());
  $post_save = (float)$usec + (float)$sec;

  // Grab these values for a few additional conversions.
  $nid = $node->nid;
  $rid = $old_release->rid;

  // While we're iterating over all the old releases, we can already
  // convert all the project_issue nodes to the new value.  We'll have
  // to fix all the followup comments only after we have the complete
  // mapping of rid -> nid
  if (module_exists('project_issue')) {
    list($usec, $sec) = explode(' ', microtime());
    $pre_update = (float)$usec + (float)$sec;
    db_query("UPDATE {project_issues} SET rid = %d WHERE rid = %d", $nid, $rid);
    list($usec, $sec) = explode(' ', microtime());
    $post_update = (float)$usec + (float)$sec;
  }

  // Keep track of it in our array in RAM for converting issue comments
  $nids_by_rid[$rid] = $nid;

  // See how long it took.
  list($usec, $sec) = explode(' ', microtime());
  $stop = (float)$usec + (float)$sec;
  $diff = round(($stop - $start) * 1000, 2);

  $save_diff = round(($post_save - $pre_save) * 1000, 2);
  $update_diff = round(($post_update - $pre_update) * 1000, 2);

  // Finally, add an entry to the {project_release_legacy} table so we
  // know the mapping of the old rid to the new nid.
  db_query("INSERT INTO {project_release_legacy} (rid, nid, pid, time, save_ms, update_ms) VALUES (%d, %d, %d, %d, %d, %d)", $rid, $nid, $old_release->pid, $diff, $save_diff, $update_diff);
  return true;
}

function convert_issue_followups() {
  if (!module_exists('project_issue')) {
    return;
  }
  global $nids_by_rid;
  $start_time = time();
  $num = 0;
  $errors = 0;

  $query = db_query("SELECT pc.*, pi.pid FROM {project_comments} pc INNER JOIN {project_issues} pi ON pc.nid = pi.nid WHERE pc.data RLIKE 'rid'");
  while ($comment = db_fetch_object($query)) {
    $error_old = 0;
    $error_new = 0;
    $data = unserialize($comment->data);
    $old_rid = $data['old']->rid;
    $new_rid = $data['new']->rid;
    if ($old_rid) {
      if (isset($nids_by_rid[$old_rid])) {
        $data['old']->rid = $nids_by_rid[$old_rid];
      }
      else {
        $error_old = $old_rid;
        $data['old']->rid = 0;
      }
    }
    if ($new_rid) {
      if (isset($nids_by_rid[$new_rid])) {
        $data['new']->rid = $nids_by_rid[$new_rid];
      }
      else {
        $error_new = $new_rid;
        $data['new']->rid = 0;
      }
    }
    if ($error_old || $error_new) {
        // Evil, this comment refers to a rid that wasn't in
        // {project_releases}, record it for post-mortem analysis...
      db_query("INSERT INTO {project_comments_conversion_errors} (cid, pid, old_rid, new_rid) VALUES (%d, %d, %d, %d)", $comment->cid, $comment->pid, $error_old, $error_new);
      $errors++;
    }
    db_query("UPDATE {project_comments} SET data = '%s' WHERE cid = %d", serialize($data), $comment->cid);
    $num++;
  }
  print t('Converted %num issue followups in %interval', array('%num' => $num, '%interval' => format_interval(time() - $start_time))) .'<br />';
  if ($errors) {
    print '<b>'. t('ERROR: problem during conversion of %num issue followups', array('%num' => $errors)) .'</b><br />';
  }
}

/**
 * Updates the {project_projects} table to fix the the "version" field
 * (the default release for download) to use the nid for the release
 * node that the old rid was turned into.
 */
function convert_default_downloads() {
  db_query("UPDATE {project_projects} pp, {project_release_legacy} prl SET pp.version = prl.nid WHERE pp.version = prl.rid");
}

function create_legacy_tables() {
  switch ($GLOBALS['db_type']) {
    case 'mysql':
    case 'mysqli':
      db_query("CREATE TABLE IF NOT EXISTS {project_release_legacy} (
        rid int(10) unsigned NOT NULL default '0',
        nid int(10) unsigned NOT NULL default '0',
        pid int(10) unsigned NOT NULL default '0',
        time int(10) unsigned NOT NULL default '0',
        save_ms int(10) unsigned NOT NULL default '0',
        update_ms int(10) unsigned NOT NULL default '0',
        PRIMARY KEY (`rid`),
        KEY project_release_legacy_pid (`pid`),
        KEY project_release_legacy_nid (`nid`)
        ) /*!40100 DEFAULT CHARACTER SET utf8 */;");
      db_query("CREATE TABLE IF NOT EXISTS {project_comments_conversion_errors} (
        cid int(10) unsigned NOT NULL default '0',
        pid int(10) unsigned NOT NULL default '0',
        old_rid int(10) NOT NULL default '-1',
        new_rid int(10) NOT NULL default '-1',
        PRIMARY KEY (`cid`),
        KEY project_comments_conversion_errors_pid (`pid`),
        KEY project_comments_conversion_errors_old_rid (`old_rid`),
        KEY project_comments_conversion_errors_new_rid (`new_rid`)
        ) /*!40100 DEFAULT CHARACTER SET utf8 */;");
      break;
    case 'pgsql':
      if (!project_db_table_exists('project_release_legacy')) {
        db_query("CREATE TABLE {project_release_legacy} (
          rid int(10) unsigned NOT NULL default '0',
          nid int(10) unsigned NOT NULL default '0',
          pid int(10) unsigned NOT NULL default '0',
          time int(10) unsigned NOT NULL default '0',
          save_ms int(10) unsigned NOT NULL default '0',
          update_ms int(10) unsigned NOT NULL default '0',
          PRIMARY KEY (`rid`),
          KEY project_release_legacy_pid (`pid`),
          KEY project_release_legacy_nid (`nid`)
          );");
      }
      if (!project_db_table_exists('project_comments_conversion_errors')) {
        db_query("CREATE TABLE {project_comments_conversion_errors} (
          cid int(10) unsigned NOT NULL default '0',
          pid int(10) unsigned NOT NULL default '0',
          old_rid int(10) NOT NULL default '-1',
          new_rid int(10) NOT NULL default '-1',
          PRIMARY KEY (`cid`),
          KEY project_comments_conversion_errors_pid (`pid`),
          KEY project_comments_conversion_errors_old_rid (`old_rid`),
          KEY project_comments_conversion_errors_new_rid (`new_rid`)
          );");
      }
      break;
  }
}

function populate_project_release_api_taxonomy() {
  $vid = _project_release_get_api_vid();

  // First, customize the vocabulary itself for our needs:
  $vocab['vid'] = $vid;
  $vocab['name'] = 'Drupal Core compatibility';
  $vocab['nodes']['project_release'] = 1;
  $vocab['help'] = 'Specify what version of Drupal Core this release is compatible with.';
  $vocab['hierarchy'] = 0;
  $vocab['required'] = 1;
  $vocab['weight'] = -5;
  $vocab['module'] = 'project_release';
  taxonomy_save_vocabulary($vocab);

  // Now, populate the terms we'll need:
  $terms[] = '5.x';
  for ($i=7; $i>=0; $i--) {
    $terms[] = "4.$i.x";
  }
  foreach ($terms as $weight => $name) {
    $edit = array();
    $edit['vid'] = $vid;
    $edit['name'] = $name;
    $edit['description'] = "Releases of Drupal contributions that are compatible with version $name of Drupal Core";
    $edit['weight'] = $weight;
    taxonomy_save_term($edit);
  }
}


/*
 *------------------------------------------------------------
 * Real work of this script
 *------------------------------------------------------------
 */
include_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// If not in 'safe mode', increase the maximum execution time:
if (!ini_get('safe_mode')) {
  set_time_limit(2000);
}

if (!module_exists('project_release')) {
  print '<b>' . t('ERROR: project_release_update.php requires that you first install the project_release.module') . '</b>';
  exit(1);
}

// Pull in the copy of project_db_table_exists()
$path = drupal_get_path('module', 'project');
if (file_exists("$path/project.install")) {
  require_once "$path/project.install";
}

$nids_by_rid = array();

populate_project_release_api_taxonomy();

create_legacy_tables();

populate_project_release_projects();

convert_all_releases();

convert_issue_followups();

convert_default_downloads();

// TODO: more user feedback, progress, etc.
// TODO: LOCK relevant tables during conversion?
