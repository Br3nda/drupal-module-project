<?php
/* $Id: release.inc,v 1.42.2.1 2005/01/13 23:20:58 unconed Exp $ */

function project_release_scan() {
  project_release_scan_directory();
  drupal_set_title(t('Release directory scan'));
  print theme('page', t('Scan successful.'));
}

function project_release_scan_directory($project = NULL) {
  if ($dir = file_create_path(variable_get('project_release_directory', ''))) {
    if ($project) {
      $regexp = "($project)" .'-(.+)\.tar\.gz';
    }
    else {
      $regexp = '(.+)-(.+)\.tar\.gz';
    }
    file_scan_directory($dir, $regexp, array('.', '..', 'CVS'), 'project_release_scan_parse');

    foreach (project_release_scan_parse() as $project) {
      if ($project->releases) {
        foreach ($project->releases as $rid) {
          project_release_delete($rid);
        }
      }
    }
  }
}

function project_release_scan_parse($path = NULL) {
  static $projects = array();

  if (is_null($path)) {
    return $projects;
  }
  else {
    $dir = dirname($path);
    $file = basename($path);
    preg_match('/^(.+?)-([0-9.]+(?:-.*)|[^-]+)\.tar\.gz$/', $file, $matches);
    list($filename, $name, $version) = $matches;

    if (!$projects[$name]) {
      if ($project = db_fetch_object(db_query("SELECT n.nid, n.type, n.moderate FROM {node} n INNER JOIN {project_projects} p USING (nid) WHERE p.uri = '%s'", $name))) {
        if ($releases = project_release_load($project, 0)) {
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
    $release->rid = $project->releases[$version];
    $release->version = $version;
    $release->path = $path;
    project_release_save($release);
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
      $node = node_load(array('nid' => arg(1)));
      $breadcrumb = array(l(t('Home'), NULL), l(t('Projects'), 'project'));
      if (function_exists('taxonomy_node_get_terms') && $term = reset(taxonomy_node_get_terms($node->nid))) {
        $breadcrumb[] = l($term->name, 'project', NULL, "tid=$term->tid");
      }
      $breadcrumb[] = l($node->title, "node/$node->nid");
      $error = 1;
      $release->nid = $node->nid;
      $output = form(project_release_form($release, $param), ($param['method'] ? $param['method'] : 'post'), $param['action'], $param['options']);

      drupal_set_title(t('Add release to %project', array('%project' => $node->title)));
      drupal_set_breadcrum($breadcrumb); 
      print theme('page', $output, $title, $breadcrumb);
    case 'edit':
      if ($release = project_release_load(arg(4))) {
        $node = node_load(array('nid' => $release->nid));
        $output = project_release_view($release);
        $output .= form(project_release_form($release, $param), ($param['method'] ? $param['method'] : 'post'), $param['action'], $param['options']);
        drupal_set_title(t('Edit release %version for %project', array('%version' => $release->version, '%project' => $node->title)));
        print theme('page', $output);
      }
      break;
    case 'delete':
      if (($release = project_release_load(arg(4))) && $release->nid == arg(1)) {
        project_release_delete(arg(4));
        drupal_goto(url("node/$release->nid/release"));
      }
      break;
    case t('Preview'):
      $edit = array2object($_POST['edit']);
      $node = node_load(array('nid' => $edit->nid));
      if (node_access('update', $node)) {
        project_release_validate($edit);
        $output = project_release_view($edit);
        $output .= form(project_release_form($edit, $param), ($param['method'] ? $param['method'] : 'post'), $param['action'], $param['options']);

        print theme('page', $output);
      }
      break;
    case t('Submit'):
      $edit = array2object($_POST['edit']);
      $node = node_load(array('nid' => $edit->nid));
      if (node_access('update', $node)) {
        project_release_validate($edit);
        if (form_get_errors()) {
          $output = project_release_form($edit, $param);

          print theme('page', form($output, ($param['method'] ? $param['method'] : 'post'), $param['action'], $param['options']));
        }
        else {
          // Save the release and re-direct to the overview page for the project
          $edit->status = 1;
          project_release_save($edit);
          drupal_goto(url("node/$node->nid/release"));
        }
      }
  }
}

function project_release_page() {
  switch ($_POST['op'] ? $_POST['op'] : arg(2)) {
    case 'view':
      if ($release = project_release_load(arg(3))) {
        $node = node_load(array('nid' => $release->nid));
        drupal_set_title("$node->title $release->version");
        print theme('page', project_release_view($release));
      }
      break;
    default:
      print theme('page', project_release_overview(arg(2)));
  }
}

function project_release_overview($arg = 0) {
  $output = '<div class="project">';
  $output .= '<div class="downloads">';

  $header = array(t('Project'), t('Downloads'), t('Description'));

  $result = db_query("SELECT DISTINCT(n.nid), n.title, n.teaser, r.tid FROM {node} n LEFT JOIN {term_node} r ON n.nid = r.nid WHERE n.type = 'project_project' AND n.status = 1 ORDER BY r.tid, n.title");

  while ($node = db_fetch_object($result)) {
    if ($tid != ($tid = $node->tid)) {
      if (count($rows)) {
        $output .= "<h2>$term->name</h2>";
        $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
        $output .= theme('table', $header, $rows);
      }
      $term = taxonomy_get_term($tid);
      $rows = array();
    }
    $releases = array();
    $version = variable_get('project_release_overview', -1);
    if ($arg || $version != -1) {
      $result2 = db_query("SELECT path, version, changed FROM {project_releases} WHERE nid = %d AND status = 1 AND path <> '' AND version LIKE '%s%%' ORDER BY created DESC", $node->nid, ($arg ? $arg : $version));
    }
    else {
      $result2 = db_query("SELECT path, version, changed FROM {project_releases} WHERE nid = %d AND status = 1 AND created > 0 AND path <> '' ORDER BY created DESC", $node->nid);
    }
    if (db_num_rows($result2)) {
      while ($release = db_fetch_object($result2)) {
        $releases[] = l($release->version, $release->path, array('class' => count($releases) ? 'project-old' : 'project-recent')) . ($release->changed > time() - 14 * 24 * 60 * 60 ? theme('mark') : '');
      }
      $rows[] = array(
        array('class' => 'project-name', 'data' => l($node->title, "node/$node->nid")),
        array('class' => 'project-download', 'data' => theme('item_list', $releases)),
        array('class' => 'project-teaser', 'data' => check_output($node->teaser)),
      );
    }
  }
  if (count($rows)) {
    $output .= "<h2>$term->name</h2>";
    $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
    $output .= theme('table', $header, $rows);
  }

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}

function project_release_form(&$release, &$param) {
  /* TODO: pending final fileapi
  // Set form parameters so we can accept file uploads.
  $param['options'] = array('enctype' => 'multipart/form-data');
  */

  $output = form_textfield(t('Version'), 'version', $release->version, 40, 255, null, null, true);
  $output .= form_textarea(t('Changes'), 'changes', $release->changes, 40, 10, null, null, true);
  if (!variable_get('project_release_directory', '')) {
  /* TODO: pending final fileapi
  $output .= form_file(t('File'), 'file', 40, ($release->fid) ? t('A file already exists, if you upload another file the current file will be replaced.') : '' . $error['file'], $release->fid);
  */
  }
  if ($release->rid) {
    $output .= form_hidden('rid', $release->rid);
  }
  $output .= form_hidden('nid', $release->nid);

  $output .= form_submit(t('Preview'));
  if (!form_get_errors()) {
    $output .= form_submit(t('Submit'));
  }

  return $output;
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
    $project = node_load(array('nid' => $project->nid));
  }

  $links = array();
  if ($release->rid && node_access('update', $project)) {
    $links[] = l('edit', "node/$project->nid/release/edit/$release->rid");
    $links[] = l('delete', "node/$project->nid/release/delete/$release->rid");
  }

  if ($release->path) {
    $output .= '<small>Download: '. l(basename($release->path), $release->path) .'</small><br />';
  }

  if ($release->created) {
    $output .= '<small>Released: '. format_date($release->created) .'</small><br />';
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
      $output .= theme('box', l(t('%project %version', array('%project' => $project->title, '%version' => $release->version)), 'node/'. $project->nid .'/release', array('id' => 'version-'. $release->version), NULL, $release->version), project_release_view($release, $project));
    }
    return $output;
  }
}

function project_release_load($rid, $status = 1) {
  if ($rid->type == 'project_project') {
    $status = $status ? 'AND status = 1' : '';
    $result = db_query("SELECT rid,version FROM {project_releases} WHERE nid = %d $status ORDER BY created DESC", $rid->nid);
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
  $fields = array('rid', 'nid', 'fid', 'path', 'created', 'changed', 'version', 'changes', 'status');
  $node = node_load(array('nid' => $release->nid));

  /* TODO: pending final fileapi
  $file = check_file('file');
  */
  if (empty($release->rid)) {
    watchdog('special', "project: added release '$release->version' to $node->title", l('view', "node/$release->nid"));
    $release->rid = db_next_id('project_rid');
    if (!variable_get('project_release_directory', '') && $file->filename) {
      $release->fid = drupal_file_save($file);
    }

    $release->created = ($release->version == 'cvs') ? 0 : time();
    $release->changed = time();

    // Prepare the query:
    foreach ($release as $key => $value) {
      if (in_array($key, $fields)) {
        $k[] = check_query($key);
        $v[] = $value;
        $s[] = "'%s'";
      }
    }

    // Insert the node into the database:
    db_query('INSERT INTO {project_releases} ('. implode(', ', $k) .') VALUES('. implode(', ', $s) .')', $v);
  }
  else {
    watchdog('special', "project: updated release '$node->title $release->version'", l('view', "node/$release->nid"));
    /* TODO: pending final fileapi
    // Replace current file if new file upload exists.
    if (!variable_get('project_release_directory', '') && $file->filename) {
      $file->fid = $release->fid;
      drupal_file_save($file);
    }
    */

    unset($release->created);
    if (!$release->scan) {
      $release->changed = time();
    }

    // Prepare the query:
    foreach ($release as $key => $value) {
      if (in_array($key, $fields)) {
        $q[] = check_query($key) ." = '%s'";
        $v[] = $value;
      }
    }
    $v[] = $release->nid;
    $v[] = $release->rid;

    db_query('UPDATE {project_releases} SET '. implode(', ', $q) .' WHERE nid = %d AND rid = %d', $v);
  }
  return $release->rid;
}

function project_release_delete($rid) {
  watchdog('special', "project: deleted release '$release->version'", l('view', "node/$release->nid"));
  db_query('UPDATE {project_releases} SET status = 0 WHERE rid = %d', $rid);
}

function project_releases_list() {
  if ($versions = cache_get('project_releases')) {
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
  }

  return $versions;
}

?>
