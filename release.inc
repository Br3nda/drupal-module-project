<?php
/* $Id: release.inc,v 1.69 2006/04/22 16:44:46 dww Exp $ */

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

function project_release_cron() {
  // TODO: should be able to optimize this to use less file system calls.
  if ($dir = variable_get('project_release_directory', '')) {
    project_release_scan_directory();
  }
}

function project_release_submit() {
  switch ($_POST['op'] ? $_POST['op'] : arg(3)) {
    case 'add':
      $node = node_load(arg(1));
      $breadcrumb = array(l(t('Home'), NULL), l(t('Projects'), 'project'));
      $taxonomy_terms = taxonomy_node_get_terms($node->nid);
      if (function_exists('taxonomy_node_get_terms') && $term = reset($taxonomy_terms)) {
        $breadcrumb[] = l($term->name, 'project', NULL, "tid=$term->tid");
      }
      $breadcrumb[] = l($node->title, "node/$node->nid");
      $error = 1;
      $release->nid = $node->nid;
      $output = drupal_get_form('project_release_form', project_release_form($release, $param));

      drupal_set_title(t('Add release to %project', array('%project' => $node->title)));
      drupal_set_breadcrumb($breadcrumb);
      return $output;
      break;
    case 'edit':
      if ($release = project_release_load(arg(4))) {
        $node = node_load($release->nid);
        $output = project_release_view($release);
        $output = drupal_get_form('project_release_form', project_release_form($release, $param));
        drupal_set_title(t('Edit release %version for %project', array('%version' => $release->version, '%project' => $node->title)));
        return $output;
      }
      break;
    case 'delete':
      if (($release = project_release_load(arg(4))) && $release->nid == arg(1)) {
        project_release_delete(arg(4));
        drupal_goto("node/$release->nid/release");
      }
      break;
    case t('Submit'):
      $edit = (object) $_POST['edit'];
      $node = node_load($edit->nid);
      if (node_access('update', $node)) {
        project_release_validate($edit);
        if (form_get_errors()) {
          $output = drupal_get_form('project_release_form', project_release_form($release, $param));
          
          return $output;
        }
        else {
          // Save the release and re-direct to the overview page for the project
          $edit->status = 1;
          project_release_save($edit);
          drupal_goto("node/$node->nid/release");
        }
      }
  }
}

/**
 * Implementation of hook_form().
 */
function project_release_form(&$release, &$param) {
  /* TODO: pending final fileapi
  // Set form parameters so we can accept file uploads.
  $param['options'] = array('enctype' => 'multipart/form-data');
  */

  $form['version'] = array(
    '#type' => 'textfield',
    '#title' => t('Version'),
    '#default_value' => $release->version,
    '#size' => 40,
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  $form['changes'] = array(
    '#type' => 'textarea',
    '#title' => t('Changes'),
    '#default_value' => $release->changes,
    '#rows' => 10,
    '#cols' => 40,
    '#required' => TRUE,
  );
  if (!variable_get('project_release_directory', '')) {
    $form['path'] = array(
      '#type' => 'file',
      '#title' => t('File'),
      '#size' => 40,
      '#description' => ($release->fid) ?
        t('A file already exists, if you upload another file the current file will be replaced.') :
        '' . $error['file'],
      '#required' => $release->fid,
    );
  }
  if ($release->rid) {
    $form['rid'] = array(
      '#type' => 'hidden',
      '#default_value' => $release->rid,
    );
  }
  $form['nid'] = array(
    '#type' => 'hidden',
    '#default_value' => $release->nid,
  );
  
  if (!form_get_errors()) {
    $form['submit'] = array(
      '#type' => 'button',
      '#value' => t('Submit'),
    );
  }

  return $form;
}

function project_release_validate(&$edit) {
  if ($edit->rid) {
    $release = project_release_load($edit->rid);
    $edit->nid = $release->nid;
    $edit->fid = $release->fid;
  }

  if (empty($edit->version)) {
    form_set_error('version', t('You must specify a valid version.'));
  }
  else {
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d AND version = '%s'", $edit->nid, trim($edit->version));
    if ($release = db_fetch_object($result)) {
      if ($edit->rid != $release->rid) {
        if ($release->status) {
          form_set_error('version', t('This version already exists for this project.'));
        }
        else {
          $edit->rid = $release->rid;
        }
      }
    }
  }

  if (empty($edit->changes)) {
    form_set_error('changes', t('You must specify changes.'));
  }

  /* TODO: pending final fileapi
  $file = check_file('file');

  if ($file && !$file->fid && !empty($file->error)) {
    $error['file'] = theme('error', $file->error);
  }
  else {
    $edit->fid = $file->fid;
  }
  */
}

function project_release_view($release, $project = 0) {
  if (!$project) {
    $project = node_load($release->nid);
  }

  $links = array();
  if ($release->rid && node_access('update', $project)) {
    $links[] = l('edit', "node/$project->nid/release/edit/$release->rid");
    //$links[] = l('delete', "node/$project->nid/release/delete/$release->rid");
  }

  if ($release->path) {
    $output .= '<small>' . t('Download: <a href="%path">%path</a>', array('%path' => base_path() . $release->path)) . '</small><br />';
    $output .= '<small>' . t('Size: %size', array('%size' => format_size(filesize($release->path)))) . '</small><br />';
    $output .= '<small>' . t('md5_file hash: %hash', array('%hash' => $release->hash)) . '</small><br />';
  }
  if ($release->created) {
    $output .= '<small>' . t('First released: %created', array('%created' => format_date($release->created))) . '</small><br />';
  }
  if ($release->changed  && ($release->changed != $release->created)) {
    $output .= '<small>' . t('Last updated: %changed', array('%changed' => format_date($release->changed))) . '</small><br />';
  }

  $output .= $release->changes;
  $output .= theme('links', $links);

  return $output;
}

function project_release_list($project, $main = 0) {
  $result = db_query('SELECT * FROM {project_releases} WHERE nid = %d AND status = 1 ORDER BY created DESC', $project->nid);
  if (db_num_rows($result)) {
    $header = array(t('Version'), t('Released'), t('Changes'));
    if (node_access('update', $project) || node_access('delete', $project)) {
      $header[] = array('data' => t('Operations'), 'colspan' => 2);
    }
    while ($release = db_fetch_object($result)) {
      $output .= theme('box', l(t('%project %version', array('%project' => $project->title, '%version' => $release->version)), 'node/'. $project->nid .'/release', array('id' => 'version-'. $release->version), NULL, 'version-'. $release->version), project_release_view($release, $project));
    }
    return $output;
  }
}

function project_release_load($rid, $status = 1) {
  if ($rid->type == 'project_project') {
    $status = $status ? 'AND status = 1' : '';
    $result = db_query("SELECT rid,version FROM {project_releases} WHERE nid = %d $status ORDER BY version DESC", $rid->nid);
    $releases = array();
    while ($release = db_fetch_object($result)) {
      $releases[$release->rid] = $release->version;
    }
    return $releases;
  }
  elseif (is_numeric($rid)) {
    return db_fetch_object(db_query('SELECT * FROM {project_releases} WHERE rid = %d ORDER BY created DESC', $rid));
  }
}

function project_release_save($release) {
  $fields = array('rid' => '%d', 'nid' => '%d', 'fid' => '%d',
                    'path' => "'%s'", 'created' => '%d',
                    'changed' => '%d', 'hash' => '"%s"', 'version' => "'%s'",
                    'changes' => "'%s'", 'status' => '%d');

  $node = node_load($release->nid);

  /* TODO: pending final fileapi
  $file = check_file('file');
  */
  // If the version was not previously released, insert it.
  if (empty($release->rid)) {

    // Make sure this release version doesn't already exist for the given project.
    // This test should not be needed, but duplicate releases on drupal.org suggest
    // that an unidentified problem is causing duplicate versions to be saved.
    if (db_num_rows(db_fetch_object(db_query("SELECT rid FROM {project_releases} WHERE nid = %d AND version = '%s'", $release->nid, $release->version)))) {
      return FALSE;
    }
    $release->rid = db_next_id('project_rid');
    if (!variable_get('project_release_directory', '') && $file->filename) {
      $release->fid = drupal_file_save($file);
    }
    if ($release->path) {
      $release->created = ($release->version == 'cvs') ? 0 : filectime($release->path);
      $release->changed = filemtime($release->path);
      $release->hash = md5_file($release->path);
    }
    else {
      $release->created = ($release->version == 'cvs') ? 0 : time();
      $release->changed = time();
    }

    // Prepare the query:
    foreach ($release as $key => $value) {
      if (in_array($key, array_keys($fields))) {
        $input[db_escape_string($key)] = $fields[$key];
        $values[] = $value;
      }
    }

    // Insert the node into the database:
    db_query('INSERT INTO {project_releases} ('. implode(', ', array_keys($input)) .') VALUES('. implode(', ', $input) .')', $values);
    watchdog('special', t("project: added release '%version' to %project.", array('%version' => $release->version, '%project' => $node->title)));
  }
  // Otherwise, update the existing record.
  else {
    /* TODO: pending final fileapi
    // Replace current file if new file upload exists.
    if (!variable_get('project_release_directory', '') && $file->filename) {
      $file->fid = $release->fid;
      drupal_file_save($file);
    }
    */

    unset($release->created);
    if($release->path) {
      if ($release->scan) {
        // Load previous md5_file hash value, and test it agains the current one to see if the file has changed.
        $previous = db_fetch_object(db_query("SELECT hash FROM {project_releases} WHERE rid = %d", $release->rid));
        if (md5_file($release->path) != $previous->hash) {
          $release->hash = md5_file($release->path);
          $release->changed = time();
        }
      }
      else {
        $release->hash = md5_file($release->path);
        $release->changed = filemtime($release->path);
      }
    }

    // Prepare the query:
    foreach ($release as $key => $value) {

      if (in_array($key, array_keys($fields))) {
        $q[] = db_escape_string($key) ." = " . $fields[$key];
        $v[] = $value;
      }
    }
    $v[] = $release->nid;
    $v[] = $release->rid;

    db_query('UPDATE {project_releases} SET '. implode(', ', $q) .' WHERE nid = %d AND rid = %d', $v);
    watchdog('special', t("project: updated release '%project %version'", array('%project' => $node->title, '%version' => $release->version)));
  }
  return $release->rid;
}

function project_release_delete($rid) {
  $release = project_release_load($rid);
  watchdog('special', t("project: unpublished release '%version'", array('%version' => $release->version)));
  db_query('UPDATE {project_releases} SET status = 0 WHERE rid = %d', $rid);
}

function project_releases_list($refresh = FALSE) {
  if (!$refresh && ($versions = cache_get('project_releases'))) {
    $versions = unserialize($versions->data);
  }
  else {
    $result = db_query("SELECT version FROM {project_releases} WHERE status = 1 GROUP BY version ORDER BY created DESC;");
    $versions = array();
    while ($object = db_fetch_object($result)) {
      $version = preg_replace('/^(.+?)\.?(?:\d+)?$/', '\1', $object->version);
      $versions[$version] = $version == $object->version ? $version : $version .'.x';
    }
    cache_set('project_releases', serialize($versions));
    menu_rebuild();
  }

  return $versions;
}
