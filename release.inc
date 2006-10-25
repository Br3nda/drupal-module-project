<?php
/* $Id: release.inc,v 1.70.2.3.2.1 2006/10/25 00:05:03 dww Exp $ */

/**
 * -- commented out; needs to be re-implemented using new system, but
 *    we need to be sure we understand what this is doing, how and why.

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
*/

/**
 * Implementation of hook_form().
 */
function project_release_form(&$release, &$param) {
  if (arg(1) == 'add') {
    if (!is_numeric(arg(3))) {
      $form['markup'] = array(
        '#value' => t('You must add releases only from projects.')
      );
      return $form;
    }
    $release->pid = arg(3);
    $project = node_load($release->pid);
    if (!$project) {
      return drupal_not_found();
    }
    $form['project'] = array(
      '#type' => 'value',
      '#value' => $project,
    );
  }
  else {
    $edit = true;
  }
  $form['#attributes'] = array("enctype" => "multipart/form-data");
 
  $form['title'] = array(
    '#type' => 'value',
    '#default_value' => $release->title,
  );

  $form['version_major'] = array(
    '#type' => 'textfield',
    '#title' => t('Major version number'),
    '#default_value' => $release->version_major,
    '#size' => 20,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  if ($edit) {
    $form['version_major']['#attributes'] = array('disabled' => $edit);
  }

  $form['version_minor'] = array(
    '#type' => 'textfield',
    '#title' => t('minor version number'),
    '#default_value' => $release->version_minor,
    '#size' => 20,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  if ($edit) {
    $form['version_minor']['#attributes'] = array('disabled' => $edit);
  }

  if (!variable_get('project_release_directory', '')) {
    $form['file'] = array(
      '#type' => 'file',
      '#title' => t('File'),
      '#size' => 40,
      '#description' => ($release->fid) ?
        t('A file already exists, if you upload another file the current file will be replaced.') :
        'Choose the file that will be associated with this release.',
    );
  }

  $form['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Changes'),
    '#default_value' => $release->body,
    '#rows' => 10,
    '#cols' => 40,
    '#required' => TRUE,
    '#description' => t('Enter a description of this version, such as a list of the major changes or updates.'),
  );
  $form['format'] = filter_form($node->format);

  $form['pid'] = array(
    '#type' => 'value',
    '#value' => $release->pid,
  );
  $form['tag'] = array(
    '#type' => 'value',
    '#value' => $release->tag,
  );
  $form['rebuild'] = array(
    '#type' => 'value',
    '#value' => $release->rebuild,
  );
  $form['file_hash'] = array(
    '#type' => 'value',
    '#value' => $release->file_hash,
  );
  $form['file_date'] = array(
    '#type' => 'value',
    '#value' => $release->file_date,
  );
  $form['file_path'] = array(
    '#type' => 'value',
    '#value' => $release->file_path,
  );
  $form['version_super_major'] = array(
    '#type' => 'value',
    '#value' => $release->version_super_major,
  );
  $form['version_super_minor'] = array(
    '#type' => 'value',
    '#value' => $release->version_super_minor,
  );
  return $form;
}

/**
 * Implementation of hook_validate().
 */
function project_release_validate(&$edit, $form) {
  $result = db_query("SELECT * FROM {project_release_node} WHERE pid = %d AND version_major = %d AND version_minor = %d AND version_super_major = %d AND version_super_minor = %d AND nid != %d", $edit->pid, $edit->version_major, $edit->version_minor, $edit->version_super_major, $edit->super_version_minor, $edit->nid);

  if ($release = db_fetch_object($result)) {
     form_set_error('version_major', t('This version already exists for this project.'));
  }

  if ($file = file_check_upload('file')) {
    $file = file_save_upload('file', file_directory_path());
    if ($file) {
      $filepath = file_create_path($file->filepath);
      form_set_value($form['file_path'], $file->filepath);
      form_set_value($form['file_date'], filemtime($filepath));
      form_set_value($form['file_hash'], md5_file($filepath));
    }
    else {
      form_set_error('file', t('There was a problem uploading the specified file.'));
    }
  }
  if ($edit->project) {
    form_set_value($form['title'], t('%project %version', array('%project' => $edit->project->title, '%version' => theme('project_release_version', (object) $edit))));
  }
}

/**
 * Implementation of hook_load().
 */
function project_release_load($node) {
  $additions = db_fetch_object(db_query("SELECT * FROM {project_release_node} WHERE nid = %d", $node->nid));
  return $additions;
}

/**
 * Implementation of hook_insert().
 */
function project_release_insert($node) {
  db_query("INSERT INTO {project_release_node} (nid, pid, tag, file_path, file_date, file_hash, rebuild, version_super_major, version_super_minor, version_major, version_minor) VALUES (%d, %d, '%s', '%s', %d, '%s', %d, %d, %d, %d, %d)", $node->nid, $node->pid, $node->tag, $node->file_path, $node->file_date, $node->file_hash, $node->rebuild, $node->version_super_major, $node->version_super_minor, $node->version_major, $node->version_minor);
}

/**
 * Implementation of hook_update().
 */
function project_release_update($node) {
  db_query("INSERT INTO {project_release_node} pid = %d, tag = '%s', file_path = '%s', file_date = %d, file_hash = '%s', rebuild = %d, version_super_major = %d, version_super_minor = %d, version_major = %d, version_minor = %d WHERE nid = %d", $node->pid, $node->tag, $node->file_path, $node->file_date, $node->file_hash, $node->rebuild, $node->version_super_major, $node->version_super_minor, $node->version_major, $node->version_minor, $node->nid);
}

/**
 * Implementation of hook_delete().
 */
function project_release_delete($node) {
  if ($node->file_path) {
    file_delete(file_create_path($node->file_path));
  }
  db_query("DELETE FROM {project_release_node} WHERE nid = %d", $node->nid);
}


/**
 * @themeable
 * Provide a proper full version name of a file.
 */
function theme_project_release_version($release) {
  if ($release->version_super_major) {
    return $release->version_super_major .'.'. $release->version_super_minor .'-'. $release->version_major .'.'. $release->version_minor;
  }
  else {
    return $release->version_major .'.'. $release->version_minor;
  }
}

/**
 * Implementation of hook_view().
 */
function project_release_view(&$release, $teaser = FALSE, $page = FALSE) {
  $project = node_load($release->pid);

  if ($page) {
    // Breadcrumb navigation
    $breadcrumb[] = array('path' => 'node/'. $project->nid, 'title' => $project->title);
    $breadcrumb[] = array('path' => 'node/'. $node->nid, 'title' => $node->title);
    $breadcrumb = project_project_set_location($project, $breadcrumb);
    menu_set_location($breadcrumb);
  }

  $links = array();
  if ($release->file_path) {
    $output .= '<small>' . t('Download: %file', array('%file' => l(basename($release->file_path), file_create_url($release->file_path)))) . '</small><br />';
    $output .= '<small>' . t('Size: %size', array('%size' => format_size(filesize(file_create_path($release->file_path))))) . '</small><br />';
    $output .= '<small>' . t('md5_file file_hash: %file_hash', array('%file_hash' => $release->file_hash)) . '</small><br />';
  }
  if ($release->created) {
    $output .= '<small>' . t('First released: %created', array('%created' => format_date($release->created))) . '</small><br />';
  }
  if ($release->changed  && ($release->changed != $release->created)) {
    $output .= '<small>' . t('Last updated: %changed', array('%changed' => format_date($release->changed))) . '</small><br />';
  }

  $output .= $release->changes;
  $output .= theme('links', $links);

  $release->body .= '<div class="project-release">' . $output . '</div>';
}

/**
 * Get an array of release nodes
 */
function project_release_get_releases($project, $nodes = true) {
  $result = db_query(db_rewrite_sql("SELECT n.nid, r.* FROM {node} n INNER JOIN {project_release_node} r ON r.nid = n.nid WHERE r.pid = %d ORDER BY n.created DESC"), $project->nid);
  $releases = array();
  while ($obj = db_fetch_object($result)) {
    if ($nodes) {
      $releases[$obj->nid] = node_load($obj->nid);
    }
    else {
      $releases[$obj->nid] = theme('project_release_version', $obj);
    }
  }
  return $releases;
}

function project_release_list($project, $main = 0) {
  if ($releases = project_release_get_releases($project)) {
    foreach ($releases as $release) {
      $output .= node_view($release);
    }
    return $output;
  }
}

function project_releases_list($refresh = FALSE) {
  if (!$refresh && ($versions = cache_get('project_releases'))) {
    $versions = unserialize($versions->data);
  }
  else {
    $result = db_query("SELECT version FROM {project_releases} WHERE status = 1 GROUP BY version, created ORDER BY created DESC;");
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
