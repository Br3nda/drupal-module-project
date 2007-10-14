<?php
// $Id: project.inc,v 1.118 2007/10/14 18:34:50 dww Exp $
// $Name:  $

/**
 * Implementation of hook_form().
 */
function project_project_form($node) {
  global $user;
  project_project_set_breadcrumb($node);

  $form['#prefix'] = '<div class="project">';

  /* Project taxonomy */
  if (project_use_taxonomy()) {
    drupal_add_js(drupal_get_path('module', 'project') .'/project.js');
    $tree = taxonomy_get_tree(_project_get_vid());
    $top_level = array();
    $options = array();
    foreach ($tree as $i => $term) {
      if ($term->parents[0] == 0) {
        $last_top = $term->tid;
        $top_level[$term->tid] = check_plain($term->name);
      }
      else {
        $options[$last_top][$term->tid] = $term->name;
      }
    }
    // See if there are any project specific taxonomy terms already
    // saved in this node (i.e. we're editing an existing project) and
    // if so, extract the right default values for our custom form
    // elements...
    if ($node->taxonomy) {
      foreach ($node->taxonomy as $tid => $obj) {
        if ($top_level[$tid]) {
          $current_top = $tid;
        }
        else {
          $current_options[$tid] = $tid;
        }
      }
    }
    $form['project_taxonomy'] = array(
      '#type' => 'fieldset',
      '#weight' => '-30',
      '#title' => t('Project categories'),
      '#collapsible' => TRUE,
    );
    $form['project_taxonomy']['project_type'] = array(
      '#title' => t('Project type'),
      '#type' => 'radios',
      '#prefix' => '<div class="project-taxonomy-element">',
      '#suffix' => '</div>',
      '#options' => $top_level,
      '#default_value' => $current_top,
      '#required' => TRUE,
    );
    $select_size = max(5, 2*count($top_level));
    foreach ($options as $tid => $values) {
      $form['project_taxonomy']["tid_$tid"] = array(
        '#title' => t('!type categories', array('!type' => $top_level[$tid])),
        '#type' => 'select',
        '#multiple' => TRUE,
        '#options' => $values,
        '#default_value' => $current_options,
        '#attributes' => array('size' => min($select_size, count($values))),
        '#prefix' => '<div class="project-taxonomy-element">',
        '#suffix' => '</div>',
      );
    }
  }

  /* Project properties */
  $form['project'] = array(
    '#type' => 'fieldset',
    '#title' => t('Project information'),
    '#collapsible' => TRUE,
  );
  $form['project']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Full project name'),
    '#default_value' => isset($node->title) ? $node->title : NULL,
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  $form['project']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Full description'),
    '#default_value' => isset($node->body) ? $node->body : NULL,
    '#cols' => 40,
    '#rows' => 10,
    '#required' => TRUE,
  );
  $form['project']['format'] = filter_form($node->format);
  $form['project']['uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Short project name'),
    '#default_value' => isset($node->uri) ? $node->uri : NULL,
    '#size' => 40,
    '#maxlength' => 50,
    '#description' => t('This will be used to generate a /project/&lt;shortname&gt;/ URL for your project.'),
    '#required' => TRUE,
  );

  $description = t('E-mail address where the project owner can be contacted.');
  if (module_exists('project_issue')) {
    $description .= ' '. t('This will be used as the From: address for issue subscription e-mails.');
  }

  $form['project']['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('Project e-mail'),
    '#default_value' => isset($node->mail) ? $node->mail : $user->mail,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => $description,
    '#required' => TRUE,
  );
  $form['project']['homepage'] = array(
    '#type' => 'textfield',
    '#title' => t('Homepage'),
    '#default_value' => isset($node->homepage) ? $node->homepage : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project homepage.'),
  );
  $form['project']['documentation'] = array(
    '#type' => 'textfield',
    '#title' => t('Documentation'),
    '#default_value' => isset($node->documentation) ? $node->documentation : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project documentation.'),
  );
  $form['project']['license'] = array(
    '#type' => 'textfield',
    '#title' => t('License'),
    '#default_value' => isset($node->license) ? $node->license : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project license.'),
  );
  $form['project']['screenshots'] = array(
    '#type' => 'textfield',
    '#title' => t('Screenshots'),
    '#default_value' => isset($node->screenshots) ? $node->screenshots : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project screenshots.'),
  );
  $form['project']['changelog'] = array(
    '#type' => 'textfield',
    '#title' => t('Changelog'),
    '#default_value' => isset($node->changelog) ? $node->changelog : NULL,
     '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to changelog.'),
  );
  $form['project']['cvs'] = array(
    '#type' => 'textfield',
    '#title' => t('CVS tree'),
    '#default_value' => isset($node->cvs) ? $node->cvs : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to webcvs/viewcvs.'),
  );
  $form['project']['demo'] = array(
    '#type' => 'textfield',
    '#title' => t('Demo site'),
    '#default_value' => isset($node->demo) ? $node->demo : NULL,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to a live demo.'),
  );

  $form['#suffix'] = '</div>';
  return $form;
}

function project_project_cleanup($input) {
  return $input;
}

function project_project_validate(&$node) {
  // Bail if user hasn't done a preview yet.
  if (!isset($node->title)) {
    return $node;
  }

  // Make sure title isn't already in use
  if (db_num_rows(db_query("SELECT nid FROM {node} WHERE type = '%s' AND status = 1 AND title = '%s' AND nid <> %d", $node->type, $node->title, $node->nid))) {
    form_set_error('title', t('This project name is already in use.'));
  }

  // Validate uri.
  if (empty($node->uri)) {
    form_set_error('uri', t('A short project name is required.'));
  }
  else {
    // Make sure uri only includes valid characters
    if (!preg_match('/^[a-zA-Z0-9_-]+$/', $node->uri)) {
      form_set_error('uri', t('Please only use alphanumerical characters for the project name.'));
    }

    // Make sure uri isn't already in use, or reserved.  Includes all X from
    // project/issues/X paths used in project_issues module
    $reserved_names = array('user', 'issues', 'releases', 'rss', 'subscribe-mail', 'search', 'add', 'update_project', 'statistics', 'comments', 'autocomplete', 'cvs', 'developers');
    if (project_use_taxonomy()) {
      $terms = taxonomy_get_tree(_project_get_vid());
      foreach ($terms as $i => $term) {
        if ($term->depth == 0) {
          $reserved_names[] = strtolower($term->name);
        }
      }
    }
    if (in_array(strtolower($node->uri), $reserved_names) || db_num_rows(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s' AND nid <> %d", $node->uri, $node->nid))) {
      form_set_error('uri', t('This project name is already in use.'));
    }
  }

  // We need a description.
  if (empty($node->body)) {
    form_set_error('body', t('You must add a project description.'));
  }

  // Validate emails
  if (empty($node->mail)) {
    form_set_error('mail', t('You must specify a project e-mail.'));
  }
  elseif ($data = user_validate_mail($node->mail)) {
    form_set_error('mail', $data);
  }

  if (!empty($node->mail_digest) && ($data = user_validate_mail($node->mail_digest))) {
    form_set_error('mail_digest', $data);
  }

  if (!empty($node->mail_copy) && ($data = user_validate_mail($node->mail_copy))) {
    form_set_error('mail_copy', $data);
  }

  if (isset($node->mail_copy_filter) && is_array($node->mail_copy_filter)) {
    $node->mail_copy_filter = array_filter($node->mail_copy_filter, 'project_project_cleanup');
  }

  if (isset($node->mail_copy_filter_state) && is_array($node->mail_copy_filter_state)) {
    $node->mail_copy_filter_state = array_filter($node->mail_copy_filter_state, 'project_project_cleanup');
  }

  // Make sure all URL fields actually contain URLs.
  $fields = array(
    'homepage' => t('Homepage'),
    'changelog' => t('Changelog'),
    'cvs' => t('CVS tree'),
    'demo' => t('Demo site'),
  );
  foreach ($fields as $uri => $name) {
    if ($node->$uri && !preg_match('/^(http|https|ftp):\/\//i', $node->$uri)) {
      form_set_error($uri, t('!field is not a valid URL.', array('!field' => $name)));
    }
  }

  // Validate the project-specific sub-categories, if any...
  if (project_use_taxonomy() && $node->project_type) {
    $tree = taxonomy_get_tree(_project_get_vid());
    $top_level = array();
    foreach ($tree as $i => $term) {
      if ($term->parents[0] == 0) {
        $top_level[$term->tid] = $term->name;
      }
    }
    foreach ($top_level as $tid => $name) {
      if ($node->project_type != $tid) {
        $tid_field = 'tid_' . $tid;
        if (!empty($node->$tid_field)) {
          form_set_error($tid, t('Project type %project_type was selected, you can not use values from %invalid_type categories', array('%project_type' => $top_level[$node->project_type], '%invalid_type' => $top_level[$tid])));
        }
      }
    }
  }
}

function project_project_set_breadcrumb($node = NULL, $extra = NULL) {
  global $_menu;

  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), NULL);

  // Find out if the site has created a menu name for /project and use that
  $pid = $_menu['path index']['project'];
  $name = $_menu['items'][$pid]['title'];
  $breadcrumb[] = l($name, 'project', array('title' => t('Browse projects')));

  if (!empty($node) && project_use_taxonomy()) {
    $result = db_query(db_rewrite_sql('SELECT t.tid, t.* FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid INNER JOIN {term_node} r ON t.tid = r.tid WHERE h.parent = 0 AND t.vid = %d AND r.nid = %d', 't', 'tid'), _project_get_vid(), $node->nid);
    $term = db_fetch_object($result);
    $breadcrumb[] = l($term->name, 'project/'. $term->name);
  }

  if (is_array($extra)) {
    $breadcrumb = array_merge($breadcrumb, $extra);
  }
  elseif ($extra && !empty($node)) {
    $breadcrumb[] = l($node->title, 'node/'. $node->nid);
  }

  drupal_set_breadcrumb($breadcrumb);
}

function project_project_view($node, $teaser = false, $page = false) {
  $node = node_prepare($node, $teaser);

  if ($page) {
    // Breadcrumb navigation
    project_project_set_breadcrumb($node);

    // If theme_project_release_project_download_table is implemented, format
    // the download table.  If this function is not implemented (eg. if the
    // project_release module is not enabled), there will not be an error
    // but of course there will be no release table.
    $project_table_output = theme('project_release_project_download_table', $node);
    if (!empty($project_table_output)) {
      $node->content['download_table'] = array(
        '#value' => $project_table_output,
        '#weight' => 1,
      );
    }

    // Misc section
    $links = array();

    foreach (array('homepage' => t('Home page'), 'documentation' => t('Read documentation'), 'license' => t('Read license'), 'changelog' => t('Read complete log of changes'), 'demo' => t('Try out a demonstration'), 'screenshots' => t('Look at screenshots')) as $uri => $name) {
      if (!empty($node->$uri)) {
        $links[] = l($name, $node->$uri);
      }
    }

    if ($links) {
      $node->content['resources'] = array(
        '#value' => theme('item_list', $links, t('Resources')),
        '#weight' => 2,
      );
    }

    // Flags that indicate what kind of access to project issues to allow.
    $has_issues = module_exists('project_issue') && !empty($node->issues);
    $view_issues = $has_issues && (user_access('access project issues') || user_access('access own project issues') || user_access('administer projects'));
    $make_issues = $has_issues && node_access('create', 'project_issue');

    // Support section.
    $links = array();
    if ($view_issues) {
      $links[] = l(t('View all support requests'), 'project/issues/'. $node->uri, null, 'categories=support&states=all', null);
      $links[] = l(t('View pending support requests'), 'project/issues/'. $node->uri, null, 'categories=support', null);
      $links[] = l(t('View pending bug reports'), 'project/issues/'. $node->uri, null, 'categories=bug', null);
      $links[] = l(t('View pending feature requests'), 'project/issues/'. $node->uri, null, 'categories=feature', null);
    }
    if ($make_issues) {
      $links[] = l(t('Request support'), 'node/add/project_issue/'. $node->uri .'/support');
      $links[] = l(t('Report new bug'), 'node/add/project_issue/'. $node->uri .'/bug');
      $links[] = l(t('Request new feature'), 'node/add/project_issue/'. $node->uri .'/feature');
    }
    else {
      $links[] = theme('project_issue_create_forbidden', $node->uri);
    }

    if ($links) {
      $node->content['support'] = array(
        '#value' => theme('item_list', $links, t('Support')),
        '#weight' => 3,
      );
    }

    // Developer section
    $links = array();
    if ($view_issues) {
      $links[] = l(t('View pending patches'), 'project/issues/'. $node->uri, null, 'states=8,13,14', null);
      $links[] = l(t('View available tasks'), 'project/issues/'. $node->uri, null, 'categories=task', null);
      $links[] = l(t('View all pending issues'), 'project/issues/'. $node->uri);
    }

    if ($node->cvs) {
      $links[] = l(t('Browse the CVS repository'), $node->cvs);
    }

    if (project_use_cvs($node)) {
      $links[] = l(t('View CVS messages'), 'project/cvs/'. $node->nid);
      $links[] = l(t('Developers'), 'project/developers/'. $node->nid);
    }

    if ($links) {
      $node->content['development'] = array(
        '#value' => theme('item_list', $links, t('Development')),
        '#weight' => 4,
      );
    }
  }
  return $node;
}

function project_project_load($node) {
  $project = db_fetch_object(db_query('SELECT * FROM {project_projects} WHERE nid = %d', $node->nid));

  return $project;
}

/**
 * hook_nodeapi() implementation specific for project nodes.
 * @see project_nodeapi().
 */
function project_project_nodeapi(&$node, $op, $arg) {
  switch ($op) {
    case 'insert':
      _project_save_taxonomy($node);
      if (module_exists('path')) {
        path_set_alias("node/$node->nid", "project/$node->uri");
      }
      break;

    case 'update':
      _project_save_taxonomy($node);
      if (module_exists('path')) {
        path_set_alias("node/$node->nid");  // Clear existing alias.
        path_set_alias("node/$node->nid", "project/$node->uri");
      }
      break;
  }
}

function project_project_insert($node) {
  db_query("INSERT INTO {project_projects} (nid, uri, homepage, changelog, cvs, demo, release_directory, mail, version, screenshots, documentation, license) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s')", $node->nid, $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->mail, $node->version, $node->screenshots, $node->documentation, $node->license);
//  project_release_scan_directory($node->uri);
}

function project_project_update($node) {
  db_query("UPDATE {project_projects} SET uri = '%s', homepage = '%s', changelog = '%s', cvs = '%s', demo = '%s', release_directory = '%s', mail = '%s', version = %d, screenshots = '%s', documentation = '%s', license = '%s' WHERE nid = %d", $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->mail, $node->version, $node->screenshots, $node->documentation, $node->license, $node->nid);
//  project_release_scan_directory($node->uri);
}

function project_project_delete($node) {
  db_query('DELETE FROM {project_projects} WHERE nid = %d', $node->nid);
}

function project_project_access($op, $node) {
  global $user;

  switch ($op) {
    case 'view':
      // Since this function is shared for project_release nodes, we have to
      // be careful what node we pass to project_check_admin_access().
      if ($node->type == 'project_release') {
        $node = node_load($node->pid);
      }
      if (project_check_admin_access($node)) {
        return TRUE;
      }
      if (!user_access('access projects')) {
         return FALSE;
      }
      break;
    case 'create':
      if ($user->uid && user_access('maintain projects')) {
        // Since this CVS access checking is non-standard, we need to
        // special-case uid 1 to always allow everything.
        if ($user->uid != 1 && module_exists('cvs') && variable_get('cvs_restrict_project_creation', 1)) {
          return db_result(db_query("SELECT uid FROM {cvs_accounts} WHERE uid = %d AND status = %d", $user->uid, CVS_APPROVED)) ? TRUE : FALSE;
        }
        else {
          return TRUE;
        }
      }
      break;
    case 'update':
      if (project_check_admin_access($node)) {
        return TRUE;
      }
      break;
    case 'delete':
      if (project_check_admin_access($node, FALSE)) {
        return TRUE;
      }
      break;
  }
}

function project_project_retrieve($key = 0) {
  if ($key) {
    if (is_numeric($key)) {
      return node_load(array('nid' => $key, 'type' => 'project_project'));
    }
    else {
      $nid = db_result(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s'", $key), 0);
      if (!$nid) {
        return new StdClass();
      }
      else {
        return node_load(array('nid' => $nid, 'type' => 'project_project'));
      }
    }
  }
  return NULL;
}

function project_developers($nid = 0) {
  if ($project = node_load($nid)) {
    if (node_access('view', $project)) {
      $output = module_invoke('cvs', 'get_project_contributors', $nid);
      drupal_set_title(t('Developers for %name', array('%name' => check_plain($project->title))));
      project_project_set_breadcrumb($project, TRUE);
      return $output;
    }
    else {
      drupal_access_denied();
    }
  }
  else {
    drupal_not_found();
  }
}

function project_cvs($nid = 0) {
  if ($project = node_load($nid)) {
    if (node_access('view', $project)) {
      $_REQUEST['nid'] = $nid;
      $output = module_invoke('cvs', 'show_messages');
      drupal_set_title(t('CVS messages for %name', array('%name' => check_plain($project->title))));
      project_project_set_breadcrumb($project, TRUE);
      return $output;
    }
    else {
      drupal_access_denied();
    }
  }
  else {
    drupal_not_found();
  }
}

function _project_save_taxonomy(&$node) {
  if (project_use_taxonomy() && $node->project_type) {
    // First, clear out all terms from the project-specific taxonomy
    // in this node. We'll re-add the right ones based on what's saved.
    // This way, we're sure to clear out things that have been changed.
    $vid = _project_get_vid();
    $result = db_query('SELECT tid FROM {term_data} WHERE vid = %d', $vid);
    $items = array();
    while ($item = db_fetch_object($result)) {
      $items[] = "tid = $item->tid";
    }
    if ($items) {
      $sql = 'DELETE FROM {term_node} WHERE nid = %d AND ('. implode(' OR ', $items) . ')';
      db_query($sql, $node->nid);
    }
    $tid = $node->project_type;
    _project_db_save_taxonomy($node->nid, $tid);
    $tid_field = 'tid_' . $tid;
    if (isset($node->$tid_field)) {
      foreach ($node->$tid_field as $tid) {
        _project_db_save_taxonomy($node->nid, $tid);
      }
    }
  }
}

function _project_db_save_taxonomy($nid, $tid) {
  db_query('INSERT INTO {term_node} (nid, tid) VALUES (%d, %d)', $nid, $tid);
}
