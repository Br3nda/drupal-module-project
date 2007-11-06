<?php
// $Id: project.inc,v 1.75.2.16.2.32 2007/11/06 18:56:08 dww Exp $
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
        '#title' => t('%type categories', array('%type' => $top_level[$tid])),
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
    '#default_value' => $node->title,
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  $form['project']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Full description'),
    '#default_value' => $node->body,
    '#cols' => 40,
    '#rows' => 10,
    '#required' => TRUE,
  );
  $form['project']['format'] = filter_form($node->format);
  $form['project']['uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Short project name'),
    '#default_value' => $node->uri,
    '#size' => 40,
    '#maxlength' => 50,
    '#description' => t('This will be used to generate a /project/&lt;shortname&gt;/ URL for your project.'),
    '#required' => TRUE,
  );
  $form['project']['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('Project e-mail'),
    '#default_value' => $node->mail ? $node->mail : $user->mail,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('E-mail address where the project owners can be contacted.'),
    '#required' => TRUE,
  );
  $form['project']['homepage'] = array(
    '#type' => 'textfield',
    '#title' => t('Homepage'),
    '#default_value' => $node->homepage,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project homepage.'),
  );
  $form['project']['documentation'] = array(
    '#type' => 'textfield',
    '#title' => t('Documentation'),
    '#default_value' => $node->documentation,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project documentation.'),
  );
  $form['project']['license'] = array(
    '#type' => 'textfield',
    '#title' => t('License'),
    '#default_value' => $node->license,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project license.'),
  );
  $form['project']['screenshots'] = array(
    '#type' => 'textfield',
    '#title' => t('Screenshots'),
    '#default_value' => $node->screenshots,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to project screenshots.'),
  );
  $form['project']['changelog'] = array(
    '#type' => 'textfield',
    '#title' => t('Changelog'),
    '#default_value' => $node->changelog,
     '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to changelog.'),
  );
  $form['project']['cvs'] = array(
    '#type' => 'textfield',
    '#title' => t('CVS tree'),
    '#default_value' => $node->cvs,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to webcvs/viewcvs.'),
  );
  $form['project']['demo'] = array(
    '#type' => 'textfield',
    '#title' => t('Demo site'),
    '#default_value' => $node->demo,
    '#size' => 40,
    '#maxlength' => 255,
    '#description' => t('Link to a live demo.'),
  );

  $form['#suffix'] = '</div>';
  return $form;
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

    // Make sure uri isn't already in use, or reserved
    if (in_array($node->uri, array('user', 'issues', 'releases')) || db_num_rows(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s' AND nid <> %d", $node->uri, $node->nid))) {
      form_set_error('uri', t('This project name is already in use.'));
    }
  }

  // We need a description.
  if (empty($node->body)) {
    form_set_error('body', t('You must add a project description.'));
  }

  // Validate email.
  if (empty($node->mail)) {
    form_set_error('mail', t('You must specify a project e-mail.'));
  }
  elseif ($data = user_validate_mail($node->mail)) {
    form_set_error('mail', $data);
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
      form_set_error($uri, t('%field is not a valid URL.', array('%field' => $name)));
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
          form_set_error($tid, t('Project type %project_type was selected, you can not use values from %invalid_type categories', array('%project_type' => theme('placeholder', $top_level[$node->project_type]), '%invalid_type' => theme('placeholder', $top_level[$tid]))));
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

function project_project_view(&$node, $teaser = false, $page = false) {
  $node = node_prepare($node, $teaser);
  if ($page) {
    // Breadcrumb navigation
    project_project_set_breadcrumb($node);

    if (function_exists('project_release_project_download_table')) {
      $output = project_release_project_download_table($node);
    }

    // Misc section
    $links = array();

    foreach (array('homepage' => t('Home page'), 'documentation' => t('Read documentation'), 'license' => t('Read license'), 'changelog' => t('Read complete log of changes'), 'demo' => t('Try out a demonstration'), 'screenshots' => t('Look at screenshots')) as $uri => $name) {
      if (!empty($node->$uri)) {
        $links[] = l($name, $node->$uri);
      }
    }

    if ($links) {
      $output .= theme('item_list', $links, t('Resources'));
    }

    // Flags that indicate what kind of access to project issues to allow.
    $has_issues = module_exist('project_issue') && $node->issues;
    $view_issues = $has_issues && (user_access('access project issues') || user_access('access own project issues') || user_access('administer projects'));
    $make_issues = $has_issues && node_access('create', 'project_issue');

    // Support section.
    $links = array();

    if (module_exist('forum') && ($support_forum = variable_get('project_support_forum', ''))) {
      $links[] = l(t('Support forum'), 'forum/' . $support_forum);
    }

    if ($view_issues) {
      $links[] = l(t('View pending bug reports'), 'project/issues/'. $node->uri, null, 'categories=bug', null);
      $links[] = l(t('View pending feature requests'), 'project/issues/'. $node->uri, null, 'categories=feature', null);
    }
    if ($make_issues) {
      $links[] = l(t('Report new bug'), 'node/add/project_issue/'. $node->uri .'/bug');
      $links[] = l(t('Request new feature'), 'node/add/project_issue/'. $node->uri .'/feature');
    }

    if ($links) {
      $output .= theme('item_list', $links, t('Support'));
    }

    // Developer section
    $links = array();
    if ($view_issues) {
      $links[] = l(t('View pending patches'), 'project/issues/'. $node->uri, null, 'states=8,13,14', null);
      $links[] = l(t('View available tasks'), 'project/issues/'. $node->uri, null, 'categories=task', null);
      $links[] = l(t('View all issues'), 'project/issues/'. $node->uri);
    }

    if ($node->cvs) {
      $links[] = l(t('Browse the CVS repository'), $node->cvs);
    }

    if (project_use_cvs($node)) {
      $links[] = l(t('View CVS messages'), 'project/cvs/'. $node->nid);
      $links[] = l(t('Developers'), 'project/developers/'. $node->nid);
    }

    if ($links) {
      $output .= theme('item_list', $links, t('Development'));
    }

    $node->body .= $output;
  }
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
      if (module_exist('path')) {
        path_set_alias("node/$node->nid", "project/$node->uri");
      }
      break;

    case 'update':
      _project_save_taxonomy($node);
      if (module_exist('path')) {
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
      if (user_access('administer projects')) {
         return TRUE;
      }
      if (user_access('access own projects') && $node->uid == $user->uid) {
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
        if ($user->uid != 1 && module_exist('cvs') && variable_get('cvs_restrict_project_creation', 1)) {
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
      $node = node_load(array('nid' => $key, 'type' => 'project_project'));
    }
    else {
      $nid = db_result(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s'", $key), 0);
      if (!$nid) {
        return new StdClass();
      }
      else {
        $node = node_load(array('nid' => $nid, 'type' => 'project_project'));
      }
    }
  }
  return $node;
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
