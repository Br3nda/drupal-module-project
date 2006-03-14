<?php
/* $Id: issue.inc,v 1.155 2006/03/14 16:18:10 thehunmonkgroup Exp $ */

function project_issue_nodeapi(&$node, $op, $arg) {
  global $user;
  switch ($op) {
    case 'validate':
      $result = db_query('SELECT * FROM {project_issue_state} WHERE sid = %d ORDER BY weight', $node->sid);
      $state = db_fetch_object($result);

      // Check if user has access, or if status is default status and therefore available to all,
      // or if user is original issue poster and poster is granted access.
      // If none of these is true, set error.
      if (!(user_access('set issue status '. str_replace("'", "", $state->name)) || ($node->sid == variable_get('project_issue_default_state', 1)) || ($state->author_has && ($user->uid == $node->uid)))) {
        form_set_error('sid', t('Invalid issue status %status: you do not have permission to set this status', array('%status' => "<em>$state</em>")));
      }
      break;
  }
}

function project_issue_page() {
  switch ($_POST['op'] ? $_POST['op'] : arg(2)) {
    case 'search':
      $project = project_project_retrieve(arg(3));
      if ($project->nid && node_access('view', $project)) {
        return project_issue_query($project);
      }
      else {
        return project_issue_query();
      }
      break;
    case 'rss':
      $project = project_project_retrieve(arg(3));
      if ($project->nid && node_access('view', $project)) {
        $query = new StdClass();
        $query->projects = array($project->nid);

        header('Content-Type: text/xml');
        print project_issue_query_result($query, 'rss');
      }
      else {
        header('Content-Type: text/xml');
        print project_issue_query_result(NULL, 'rss');
      }
      break;
    case 'add':
      $project = project_project_retrieve(arg(3));
      // We assume that a user may create issues for projects that he may view.
      if ($project->nid && node_access('view', $project)) {
        drupal_goto("node/add/project_issue/$project->uri");
      }
      else {
        drupal_goto("node/add/project_issue");
      }
      break;
    case 'statistics':
      $project = project_project_retrieve(arg(3));
      if ($project->nid && node_access('view', $project)) {
        return project_issue_statistics($project);
      }
      else {
        return project_issue_statistics();
      }
      break;
    case t('Subscribe'):
    case 'subscribe':
      $project = project_project_retrieve(arg(3));
      if ($GLOBALS['user']->uid && $project->nid && node_access('view', $project)) {
        return project_issue_subscribe($project);
      }
      else {
        return project_issue_subscribe();
      }
      break;
    case 'user':
       $_GET['users'] = $GLOBALS['user']->uid;
    default:
      $project = project_project_retrieve(arg(2));
      if ($project->nid && node_access('view', $project)) {
        $query = new StdClass();
        $query->projects = array($project->nid);

        return project_issue_query_result($query);
      }
      else {
        return project_issue_query_result();
      }
  }
}

function project_issue_statistics($project = 0) {
  if ($project->nid) {
    $breadcrumb = array(
      l(t('Home'), NULL),
      l(t('Projects'), 'project')
    );
    $taxonomy_terms = taxonomy_node_get_terms($project->nid);
    if (function_exists('taxonomy_node_get_terms') && $term = reset($taxonomy_terms)) {
      $breadcrumb[] = l($term->name, 'project/'. $term->name);
    }
    $breadcrumb[] = $project->nid ? l($project->title, "node/$project->nid") : l(t('All issues'), 'project/issues');
    drupal_set_breadcrumb($breadcrumb);
    $filter = sprintf(' AND p.pid = %d ', db_escape_string($project->nid));
  }

  $output = '<div class="project">';

  // Issue lifetime
  $header = array(t('Category'), t('Overall'), t('Last month'));
  $rows = array();
  $duration = time() - 30 * 24 * 60 * 60;
  $result = db_query('SELECT p.category, SUM(n.changed - n.created) / COUNT(p.category) AS duration FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid WHERE n.status = 1%s AND p.sid > 1 GROUP BY p.category', $filter);
  while ($stat = db_fetch_object($result)) {
    $i++;
    $rows[$i][0] = project_issue_category($stat->category);;
    $rows[$i][1] = array('data' => format_interval($stat->duration, 2), 'class' => 'numeric');
  }

  $result = db_query('SELECT p.category, SUM(n.changed - n.created) / COUNT(p.category) AS duration FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid WHERE n.status = 1%s AND p.sid > 1 AND n.created > %d GROUP BY p.category', $filter, $duration);
  while ($stat = db_fetch_object($result)) {
    $j++;
    $rows[$j][2] = array('data' => format_interval($stat->duration, 2), 'class' => 'numeric');
  }
  $output .= '<h2>'. t('Average lifetime') .'</h2>';
  $output .= theme('table', $header, $rows);

  $header = array(t('Status'), t('Overall'), '%', t('Last month'));
  $rows = array();
  // Activity overall
  $total = db_result(db_query(db_rewrite_sql('SELECT COUNT(p.nid) AS total FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid WHERE n.status = 1%s', 'p'), $filter));
  $result = db_query(db_rewrite_sql('SELECT COUNT(p.nid) AS total, p.sid FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid WHERE n.status = 1%s GROUP BY p.sid', 'p'), $filter);
  while ($stat = db_fetch_object($result)) {
    $rows[$stat->sid][0] = project_issue_state($stat->sid);;
    $rows[$stat->sid][1] = array('data' => $stat->total, 'class' => 'project-numeric');
    $rows[$stat->sid][2] = array('data' => number_format($stat->total / $total * 100) .'%', 'class' => 'project-numeric2');
  }
  // Activity this week
  $result = db_query(db_rewrite_sql('SELECT COUNT(p.nid) AS total, p.sid FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid WHERE n.status = 1%s AND n.changed > %d GROUP BY p.sid', 'p'), $filter, $duration);
  while ($stat = db_fetch_object($result)) {
    $rows[$stat->sid][3] = array('data' => $stat->total, 'class' => 'project-numeric');
  }
  $output .= '<h2>'. t('Issue activity') .'</h2>';
  $output .= theme('table', $header, $rows);

  // Project overview
  if (!$project->nid) {
    $header = array_merge(array(t('Project')), project_issue_state(), array(t('Total')));
    $rows = array();
    $result = db_query(db_rewrite_sql("SELECT n.nid, n.title, p.sid, COUNT(n.nid) AS total FROM {node} n INNER JOIN {project_issues} p ON n.nid = p.pid WHERE n.type = 'project_project' AND n.status = 1 GROUP BY n.nid, p.sid, n.title"));

    $orig = array('project' => array('data' => 0));
    foreach (project_issue_state() as $key => $value) {
      $orig[$key] = array('data' => '', 'class' => 'project-numeric');
    }
    $orig['total'] = array('data' => '', 'class' => 'project-numeric');

    while ($stat = db_fetch_object($result)) {
      if (!$rows[$stat->nid]['project']) {
        $rows[$stat->nid] = $orig;
        $rows[$stat->nid]['project']['data'] = l($stat->title, "node/$stat->nid");
      }
      $rows[$stat->nid]['total']['data'] += $stat->total;
      $rows[$stat->nid][$stat->sid]['data'] = $stat->total;
    }
    usort($rows, create_function('$a,$b', 'return $a[1]["data"] < $b[1]["data"];'));
    $output .= '<h2>'. t('Project overview') .'</h2>';
    $output .= theme('table', $header, $rows);
  }

  $output .= '</div>';
  return $output;
}

function project_issue_subscribe_submit($form_id, $form_values) {

  global $user;
  $all = $_POST['all'];

  $levels = array(0 => t('None'), 1 => t('Own issues'), 2 => t('All issues'));

    // Remove previous subscriptions for user.
    if ($nid = $form_values['single']) {
      db_query('DELETE FROM {project_subscriptions} WHERE nid = %d AND uid = %d', $nid, $user->uid);
    }
    else {
      db_query('DELETE FROM {project_subscriptions} WHERE uid = %d', $user->uid);
    }

    if ($all) {
      $_level = array_search($all, $levels);
    }

    foreach ($form_values['options'] as $nid => $level) {
      if ($_level !== 0 && $level !== 0) {
        db_query('INSERT INTO {project_subscriptions} (nid, uid, level) VALUES (%d, %d, %d)', $nid, $user->uid, $_level ? $_level : $level);
      }
    }
    drupal_set_message(t('Subscription settings saved.'));
    drupal_goto('project/issues/subscribe');
}


function project_issue_subscribe($project = 0) {
  global $user;

  $levels = array(0 => t('None'), 1 => t('Own issues'), 2 => t('All issues'));

  if ($project) {
    $level = db_result(db_query('SELECT level FROM {project_subscriptions} WHERE nid = %d AND uid = %d', $project->nid, $user->uid));
    $form['single'] = array(
      '#type' => 'value',
      '#value' => $project->nid,
    );
    $form['subscribe'] = array(
      '#type' => 'markup',
      '#value' => '<p>'. t('Subscribe to receive e-mail notification when an issue for this project is updated.') .'</p>',
    );
    $form['options']['#tree'] = TRUE;
    $form['options'][$project->nid] = array(
      '#type' => 'radios',
      '#title' => t('Subscribe to %project issues', array('%project' => $project->title)),
      '#default_value' => $level,
      '#options' => $levels,
    );

  }
  else {

    $form['buttons']['all'] = array(
      '#type' => 'markup',
      '#value' => t('All projects'),
    );
    foreach ($levels as $key => $level) {
      $form['buttons'][$level] = array(
        '#type' => 'button',
        '#name' => 'all',
        '#value' => $level,
      );
    }
    $nids = array();

    $result = db_query(db_rewrite_sql("SELECT s.nid, n.title, s.level FROM {project_subscriptions} s INNER JOIN {node} n ON n.nid = s.nid WHERE n.type = 'project_project' AND n.status = 1 AND s.uid = %d ORDER BY n.title", 's'), $user->uid);
    while ($project = db_fetch_object($result)) {
      $form['project'][$project->nid]['title'] = array(
        '#type' => 'markup',
        '#value' => $project->title,
      );
      foreach ($levels as $key => $level) {
        if ($project->level == $key) {
          $status[$project->nid] = $key;
        }
      }
      $nids[] = $project->nid;
    }

    $result = db_query(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type = 'project_project' AND n.status = 1". ($nids ?  " AND n.nid NOT IN (%s)" : "") ." ORDER BY n.title"), implode(',', $nids));
    while ($project = db_fetch_object($result)) {
      $form['project'][$project->nid]['title'] = array(
        '#type' => 'markup',
        '#value' => $project->title,
      );
      $nids[] = $project->nid;
    }

    foreach ($nids as $nid) {
      $form['options']['#tree'] = TRUE;
      $form['options'][$nid] = array(
        '#type' => 'radios',
        '#default_value' => $status[$nid],
        '#options' => $levels,
      );
    }
  }
  $form['submit'] = array(
    '#type' => 'button',
    '#value' => t('Subscribe'),
  );
  return drupal_get_form('project_issue_subscribe', $form);
}

function theme_project_issue_subscribe($form) {
  global $user;

  $output = '';

  if (!$form['single']) {
    $levels = array(0 => t('None'), 1 => t('Own issues'), 2 => t('All issues'));
    $headers = array_merge(array(t('Project')), $levels);

    $row = array();
    foreach (element_children($form['buttons']) as $key) {
      $row[] = form_render($form['buttons'][$key]);
    }
    $rows = array($row);

    foreach (element_children($form['project']) as $key) {
      $row = array(form_render($form['project'][$key]['title']));
      foreach ($levels as $level => $name) {
        $row[] = form_render($form['options'][$key][$level]);
      }
      $rows[] = $row;
    }
    $output = theme('table', $headers, $rows);
  }

  $output .= form_render($form);
  return $output;
}

/**
 * Implementation of hook_form().
 */
function project_issue_form(&$node, &$param) {
  global $user;

  $default_state = variable_get('project_issue_default_state', 1);
  // Set form parameters so we can accept file uploads.
  $form['#attributes'] = array('enctype' => 'multipart/form-data');

  // Set the pre-render function for adjustments to the form stages
  $form['#pre_render'] = array('project_issue_form_pre_render');

  // Fetch a list of all projects to make swapping simpler
  $projects = array(t('<none>')) + project_projects_select_options();

  // Try to find the active project
  if (empty($node->pid) && $arg = arg(3)) {
    if (is_numeric($arg)) {
      $node->pid = db_result(db_query(db_rewrite_sql('SELECT p.nid FROM {project_projects} p WHERE p.nid = %d', 'p'), $arg), 0);
    }
    else {
      $node->pid = db_result(db_query(db_rewrite_sql("SELECT p.nid FROM {project_projects} p WHERE p.uri = '%s'", 'p'), $arg), 0);
    }
  }
  $pid = $node->pid ? $node->pid : $_POST['edit']['pid'];
  if ($pid) {
    // Load the project and initialize some support arrays.
    $project = node_load(array('nid' => $pid, 'type' => 'project_project'));
    if (trim($project->help)) {
      drupal_set_message($project->help);
    }
    if ($releases = project_release_load($project)) {
      $releases = array(t('<none>')) + $releases;
    }
    $components = array();
    if ($project->components) {
      $components = array(t('<none>'));
      foreach ($project->components as $component) {
        $components[$component] = $component;
      }
    }
    $categories = array_merge(array(t('<none>')), project_issue_category(0, 0));
    $priorities = project_issue_priority();
    $states = project_issue_state(0, true, $node->nid && ($node->uid == $user->uid));

    if ($user->uid == $node->assigned) {
      $assigned = array(0 => t('Unassign'), $user->uid => $user->name);
    }
    else {
      $assigned = array(
        $node->assigned => ($node->assigned && ($account = user_load(array('uid' => $node->assigned))) ? $account->name : t('Unassigned')),
        $user->uid => $user->name
      );
    }

    $form['#prefix'] = '</div><div class="admin">';

    $form['project_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Project information'),
      '#prefix' => '<div class="options">',
      '#suffix' => '</div>',
    );
    $form['project_info']['pid'] = array(
      '#type' => 'select',
      '#title' => t('Project'),
      '#default_value' => $node->pid,
      '#options' => $projects,
      '#required' => TRUE,
    );
    if ($releases) {
      $form['project_info']['rid'] = array(
        '#type' => 'select',
        '#title' => t('Version'),
        '#default_value' => $node->rid,
        '#options' => $releases,
        '#required' => TRUE,
      );
    }
    $form['project_info']['component'] = array(
      '#type' => 'select',
      '#title' => t('Component'),
      '#default_value' => $node->component,
      '#options' => $components,
      '#required' => TRUE,
    );

    $form['issue_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Issue information'),
      '#prefix' => '<div class="options">',
      '#suffix' => '</div>',
    );
    $form['issue_info']['category'] = array(
      '#type' => 'select',
      '#title' => t('Category'),
      '#default_value' => $node->category ? $node->category : arg(4),
      '#options' => $categories,
      '#required' => TRUE,
    );
    $form['issue_info']['priority'] = array(
      '#type' => 'select',
      '#title' => t('Priority'),
      '#default_value' => $node->priority ? $node->priority : 2,
      '#options' => $priorities,
    );
    $form['issue_info']['assigned'] = array(
      '#type' => 'select',
      '#title' => t('Assigned'),
      '#default_value' => $node->assigned,
      '#options' => $assigned,
    );
    if (count($states) > 1) {
      $form['issue_info']['sid'] = array(
        '#type' => 'select',
        '#title' => t('Status'),
        '#default_value' => $node->sid ? $node->sid : $default_state,
        '#options' => $states,
      );
    }
    else {
      $form['issue_info']['sid'] = array(
        '#type' => 'hidden',
        '#value' => $default_state,
      );
      $form['issue_info']['status'] = array(
        '#type' => 'item',
        '#title' => t('Status'),
        '#value' => project_issue_state($default_state),
      );
    }

    $form['issue_details'] = array(
      '#type' => 'fieldset',
      '#title' => t('Issue Details'),
      '#prefix' => '</div><div class="standard">',
    );
    $form['issue_details']['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#default_value' => $node->title,
      '#size' => 60,
      '#maxlength' => 128,
      '#required' => TRUE,
    );
    $form['issue_details']['body'] = array(
      '#type' => 'textarea',
      '#title' => t('Description'),
      '#default_value' => $node->body,
      '#rows' => 10,
      '#required' => TRUE,
    );
    $form['issue_details']['format'] = filter_form($node->format);
    $form['issue_details']['file_issue'] = array(
      '#type' => 'file',
      '#title' => t('File attachment'),
      '#size' => 40,
      '#description' => ($node->file || $node->file_path ? t('A file already exists, if you upload another file the current file will be replaced.') : t('Optionally attach a file, for example a patch or a screenshot.')),
    );

  }
  else {

    $form['pid'] = array(
      '#type' => 'select',
      '#title' => t('Project'),
      '#default_value' => $node->pid,
      '#options' => $projects,
      '#required' => TRUE,
    );
    // Add a field indicating that this is the first stage of a multi-stage submission.
    $form['wizard'] = array(
      '#type' => 'value',
      '#value' => 'one',
    );
  }

  return $form;
}

function project_issue_form_pre_render($form_id, &$form) {
  if ($form_id == 'project_issue_node_form') {
    // Alter the buttons, and auto validate required fields for first page
    if ($form['wizard']['#value'] == 'one') {
      $form['preview'] = array(
        '#type' => 'button',
        '#value' => t('Next'),
        '#weight' => 19,
      );
      $form['submit'] = array(
        '#type' => 'value',
        '#value' => 'hidden',
      );
    }
  }
}

function project_issue_node_form_validate($form_id, $form) {
  global $form_values;
  $edit = $_POST['edit'];
  if (!$form_values['pid']) {
    form_set_error('pid', t('You have to specify a valid project.'));
  }

  // Otherwise, validate the rest of the form if it has been previously submitted.
  if (($edit['wizard'] == 'two')) {
    if ($form_values['pid'] && $project = node_load($form_values['pid'])) {
      $node->title = $form_values['title'];
      node_validate_title($node);
      if ($releases = project_release_load($project)) {
        if (!$form_values['rid'] || !$releases[$form_values['rid']]) {
          $form_values['rid'] = $project->version;
        }
        empty($form_values['rid']) and form_set_error('rid', t('You have to specify a valid version.'));
      }
      if ($form_values['component'] && !in_array($form_values['component'], $project->components)) {
        $form_values['component'] = 0;
      }

      empty($form_values['component']) and form_set_error('component', t('You have to specify a valid component.'));
      empty($form_values['category']) and form_set_error('category', t('You have to specify a valid category.'));
      empty($form_values['body']) and form_set_error('body', t('You have to specify a valid description.'));

      $file = file_check_upload('file_issue');
      $form_values['file'] = file_save_upload($file);
    }
  }
}




function project_issue_comment_validate(&$node) {

  // Set default values.
  empty($node->priority) and $node->priority = 2;
  empty($node->category) and $node->category = arg(4);
  empty($node->sid) and $node->sid = 1;

  // Try to find the active project
  if (empty($node->pid) && $arg = arg(3)) {
    if (is_numeric($arg)) {
      $node->pid = db_result(db_query(db_rewrite_sql('SELECT p.nid FROM {project_projects} p WHERE p.nid = %d', 'p'), $arg), 0);
    }
    else {
      $node->pid = db_result(db_query(db_rewrite_sql("SELECT p.nid FROM {project_projects} p WHERE p.uri = '%s'", 'p'), $arg), 0);
    }
  }

  // Validate the rest of the form.
  if (isset($node->title) && !$node->validated) {
    if ($node->pid && $project = node_load($node->pid)) {
      node_validate_title($node);
      if ($releases = project_release_load($project)) {
        if (!$node->rid || !$releases[$node->rid]) {
          $node->rid = $project->version;
        }
        empty($node->rid) and form_set_error('rid', t('You have to specify a valid version.'));
      }
      if ($node->component && !in_array($node->component, $project->components)) {
        $node->component = 0;
      }

      empty($node->component) and form_set_error('component', t('You have to specify a valid component.'));
      empty($node->category) and form_set_error('category', t('You have to specify a valid category.'));
      empty($node->body) and !$node->comment and form_set_error('body', t('You have to specify a valid description.'));

      $file = file_check_upload('file_issue');
      $node->file = file_save_upload($file);
    }
    elseif (isset($node->pid)) {
      form_set_error('pid', t('You have to specify a valid project.'));
    }
  }

  return $node;
}




/**
 * Implementation of hook_content().
 */
function project_issue_content($node, $teaser = false) {
  return node_prepare($node, $teaser);
}

function project_issue_view(&$node, $teaser = false, $page = false) {
  $node = project_issue_content($node, $teaser);

  if (!$teaser && $page) {
    $project = node_load(array('nid' => $node->pid, 'type' => 'project_project'));
    $release = project_release_load($node->rid);
    $assigned = ($node->assigned && ($account = user_load(array('uid' => $node->assigned))) ? $account->name : t('Unassigned'));

    $output = '<div class="project">';
    $rows = array();
    $rows[] = array('Project:', $project->title);
    if ($release->version) {
      $rows[] = array('Version:', $release->version);
    }
    $rows[] = array(t('Component:'),  check_plain($node->component));
    $rows[] = array(t('Category:'), check_plain($node->category));
    $rows[] = array(t('Priority:'), project_issue_priority($node->priority));
    $rows[] = array(t('Assigned:'), $assigned);
    $rows[] = array(t('Status:'), project_issue_state($node->sid));
    if ($node->file_path && file_exists($node->file_path)) {
      $rows[] = array(t('Attachment:'), '<a href="'. file_create_url($node->file_path). '">'. basename($node->file_path) .'</a> ('. format_size($node->file_size) .')');
    }
    $output .= '<div class="summary">'. theme('table', array(), $rows) .'</div>';

    $output .= '<p>'. t('Description') .'</p>';
    $output .= '<p>'. $node->body .'</p>';
    $output .= '</div>';

    $node->body = $output;

    // Breadcrumb navigation
    $breadcrumb[] = array('path' => 'project', 'title' => t('projects'));
    $taxonomy_terms = taxonomy_node_get_terms($node->pid);
    if (function_exists('taxonomy_node_get_terms') && $term = reset($taxonomy_terms)) {
      $breadcrumb[] = array('path' => 'project/'. $term->name, 'title' => $term->name);
    }
    $breadcrumb[] = array('path' => 'node/'. $project->nid, 'title' => $project->title);
    $breadcrumb[] = array('path' => 'project/issues/'. $project->uri, 'title' => t('issues'));
    $breadcrumb[] = array('path' => 'node/'. $node->nid);
    menu_set_location($breadcrumb);

    $node->body .= project_comment_view($node);
  }
}

function project_issue_load($node) {
  $project = db_fetch_object(db_query(db_rewrite_sql('SELECT pi.* FROM {project_issues} pi WHERE pi.nid = %d', 'pi'), $node->nid));
  return $project;
}

function project_issue_insert($node) {
  if ($node->file) {
    $file = file_save_upload($node->file, variable_get('project_directory_issues', 'issues'));
  }

  db_query("INSERT INTO {project_issues} (nid, pid, category, component, priority, rid, assigned, sid, file_path, file_mime, file_size) VALUES (%d, %d, '%s', '%s', %d, %d, %d, %d, '%s', '%s', %d)", $node->nid, $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->sid, $file->filepath, $file->filemime, $file->filesize);
  project_mail_notify($node);
}

function project_issue_update($node) {
  if ($node->file) {
    // Remove old file.
    file_delete(db_result(db_query('SELECT file_path FROM {project_issues} WHERE nid = %d', $node->nid)));
    $file = file_save_upload($node->file, variable_get('project_directory_issues', 'issues'));
    db_query("UPDATE {project_issues} SET pid = %d, category = '%s', component = '%s', priority = %d, rid = %d, assigned = %d, sid = %d, file_path = '%s', file_mime = '%s', file_size = %d WHERE nid = %d", $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->sid, $file->filepath, $file->filemime, $file->filesize, $node->nid);
  }
  else {
    db_query("UPDATE {project_issues} SET pid = %d, category = '%s', component = '%s', priority = %d, rid = %d, assigned = %d, sid = %d WHERE nid = %d", $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->sid, $node->nid);
  }
  project_mail_notify($node);
}

function project_issue_delete($node) {
  file_delete($node->file_path);
  db_query('DELETE FROM {project_issues} WHERE nid = %d', $node->nid);
}

function project_issue_access($op, $node) {
  global $user;
  switch ($op) {
    case 'view':
      return user_access('access project issues') || (user_access('access own project issues') && $node->uid == $user->uid);
    case 'create':
      return user_access('create project issues');
    case 'update':
    case 'delete':
      return user_access('administer projects');
  }
}

// Support stuff
function project_issue_state($sid = 0, $restrict = false, $is_author = false) {
  static $options;

  if (!$options) {
    $result = db_query('SELECT * FROM {project_issue_state} ORDER BY weight');
    while ($state = db_fetch_object($result)) {
      $options[] = $state;
    }
  }

  foreach($options as $state) {
    if ($restrict) {
      // Check if user has access, or if status is default status and therefore available to all,
      // or if user is original issue poster and poster is granted access
      if (user_access('set issue status '. str_replace("'", "", $state->name)) || ($state->sid == variable_get('project_issue_default_state', 1)) || ($state->author_has && $is_author)) {
        $states[$state->sid] = $state->name;
      }
    }
    else {
      $states[$state->sid] = $state->name;
    }
  }

  return $sid ? $states[$sid] : $states;
}

function project_issue_priority($priority = 0) {
  $priorities = array(1 => 'critical', 'normal', 'minor');
  return $priority ? $priorities[$priority] : $priorities;
}

function project_issue_category($category = 0, $plural = 1) {
  if ($plural) {
    $categories = array('bug' => t('bug reports'), 'task' => t('tasks'), 'feature' => t('feature requests'), 'support' => t('support requests'));
  }
  else {
    $categories = array('bug' => t('bug report'), 'task' => t('task'), 'feature' => t('feature request'), 'support' => t('support request'));
  }
  return $category ? $categories[$category] : $categories;
}

function project_issue_count($pid) {
  $state = array();
  $result = db_query('SELECT p.sid, count(p.sid) AS count FROM {node} n INNER JOIN {project_issues} p ON n.nid = p.nid WHERE n.status = 1 AND p.pid = %d GROUP BY p.sid', $pid);
  while ($data = db_fetch_object($result)) {
    $state[$data->sid] = $data->count;
  }
  return $state;
}

function project_issue_query($project = 0, $query = NULL) {
  $categories = project_issue_category();
  $components = array();
  if ($project->nid) {
    foreach ($project->components as $component) {
      $components[$component] = $component;
    }
    $versions = project_release_load($project, 0);
  }
  else {
    $result = db_query(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type = 'project_project' AND n.status = 1 AND n.moderate = 0 ORDER BY n.title"));
    while ($node = db_fetch_object($result)) {
      $projects[$node->nid] = $node->title;
    }
  }
  if (is_null($query)) {
    $query = new StdClass();
  }
  $states = project_issue_state();
  $priorities = project_issue_priority();

  $form['#action'] = url($project->uri ? "project/issues/$project->uri" : 'project/issues');
  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Search for'),
    '#default_value' => $query->summary,
    '#maxlength' => 255,
  );
  $form['attachment'] = array(
    '#type' => 'checkbox',
    '#title' => t('Has attachment.'),
    '#default_value' => $query->attachment,
  );

  if ($projects) {

    $form['projects'] = array(
      '#type' => 'select',
      '#title' => t('Projects'),
      '#default_value' => $query->projects,
      '#options' => $projects,
      '#attributes' => array('size' => 5),
      '#multiple' => TRUE,
    );
    $form['categories'] = array(
      '#type' => 'select',
      '#title' => t('Categories'),
      '#default_value' => $query->categories,
      '#options' => $categories,
      '#attributes' => array('size' => 5),
      '#multiple' => TRUE,
    );
  }
  else {
    $form['versions'] = array(
      '#type' => 'select',
      '#title' => t('Versions'),
      '#default_value' => $query->versions,
      '#options' => $versions,
      '#attributes' => array('size' => 5),
      '#multiple' => TRUE,
    );
    $form['components'] = array(
      '#type' => 'select',
      '#title' => t('Components'),
      '#default_value' => $query->components,
      '#options' => $components,
      '#attributes' => array('size' => 5),
      '#multiple' => TRUE,
    );
    $form['categories'] = array(
      '#type' => 'select',
      '#title' => t('Categories'),
      '#default_value' => $query->categories,
      '#options' => $categories,
      '#attributes' => array('size' => 5),
      '#multiple' => TRUE,
    );
  }
  $form['priorities'] = array(
    '#type' => 'select',
    '#title' => t('Priorities'),
    '#default_value' => $query->priorities,
    '#options' => $priorities,
    '#attributes' => array('size' => 5),
    '#multiple' => TRUE,
  );
  $form['states'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#default_value' => ($query->states) ? $query->states : array(1, 2, 8, 13, 14),
    '#options' => $states,
    '#attributes' => array('size' => 5),
    '#multiple' => TRUE,
  );
  $form['submitted'] = array(
    '#type' => 'textfield',
    '#title' => t('Submitted by'),
    '#default_value' => $query->submitted,
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => 20,
    '#maxlength' => 255,
  );
  $form['assigned'] = array(
    '#type' => 'textfield',
    '#title' => t('Assigned'),
    '#default_value' => $query->assigned,
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => 20,
    '#maxlength' => 255,
  );
  $form['participated'] = array(
    '#type' => 'textfield',
    '#title' => t('Participant'),
    '#default_value' => $query->users,
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => 20,
    '#maxlength' => 255,
  );
  $form['submit'] = array(
    '#type' => 'button',
    '#value' => t('Search'),
  );

  return drupal_get_form('project_issue_query', $form);
}




function theme_project_issue_query($form) {

  $rows[] = array(
    array('data' => form_render($form['text']), 'colspan' => 3)
  );
  $rows[] = array(
    array('data' => form_render($form['attachment']), 'colspan' => 3)
  );
  if ($form['projects']) {
    $rows[] = array(
      form_render($form['projects']),
      form_render($form['categories']),
      NULL,
    );
  }
  else {
    $rows[] = array(
      form_render($form['versions']),
      form_render($form['components']),
      form_render($form['categories']),
    );
  }
  $rows[] = array(
    array('data' => form_render($form['priorities'])),
    array('data' => form_render($form['states']), 'colspan' => 2)
  );
  $rows[] = array(
    form_render($form['submitted']),
    form_render($form['assigned']),
    form_render($form['participated'])
  );
  $rows[] = array(
    array('data' => form_render($form['submit']), 'colspan' => 3)
  );

  $output = '<div class="project">';
  $output .= theme('table', array(), $rows);
  $output .= '</div>';
  $output .= form_render($form);

  return $output;
}

function theme_project_issue_admin_states_page($form) {
  $header = array(
        array('data' => t('ID')),
        array('data' => t('Name')),
        array('data' => t('Weight')),
        array('data' => t('Author may set')),
        array('data' => t('Default')),
        array('data' => t('Operations'))
      );
  foreach (element_children($form['status']) as $key) {
    $rows[] = array(form_render($form['status'][$key]['sid']),
                    form_render($form['status'][$key]['name']),
                    form_render($form['status'][$key]['weight']),
                    form_render($form['status'][$key]['author_has']),
                    form_render($form['default_state'][$key]),
                    form_render($form['delete'][$key]));
  }
  $rows[] = array(NULL, form_render($form['status_add']['name']), form_render($form['status_add']['weight']), NULL, NULL, NULL);
  $output = '<div>' . theme('table', $header, $rows) . '</div>';
  $output .= form_render($form);
  return $output;
}

function project_issue_admin_states_page() {
  switch ($_POST['op'] ? $_POST['op'] : arg(4)) {
    case 'save':
    case t('Save'):
      project_issue_admin_save_states();
      break;
    case 'delete':
    case t('Delete'):
      return project_issue_admin_delete_state_confirm();
      break;
    default:
      $result = db_query('SELECT * FROM {project_issue_state} ORDER BY weight');
      $default_state = variable_get('project_issue_default_state', 1);
      $form['status']['#tree'] = TRUE;
      while ($state = db_fetch_object($result)) {
        $options[$state->sid] = '';
        $form['status'][$state->sid]['sid'] = array(
          '#type' => 'markup',
          '#value' => $state->sid,
        );
        $form['status'][$state->sid]['name'] = array(
          '#type' => 'textfield',
          '#default_value' => $state->name,
          '#size' => 20,
          '#maxlength' => 255,
        );
        $form['status'][$state->sid]['weight'] = array(
          '#type' => 'weight',
          '#default_value' => $state->weight,
          '#delta' => 15,
        );
        $form['status'][$state->sid]['author_has'] = array(
          '#type' => 'checkbox',
          '#default_value' => $state->author_has,
        );
        $form['delete'][$state->sid] = array(
          '#type' => 'markup',
          '#value' => ($state->sid != $default_state) ?
            l(t('delete'), 'admin/settings/project/status/delete/'. $state->sid) :
            '',
        );
      }
      $form['default_state'] = array(
        '#type' => 'radios',
        '#options' => $options,
        '#default_value' => $default_state,
      );
      $form['status_add']['#tree'] = TRUE;
      $form['status_add']['name'] = array(
        '#type' => 'textfield',
        '#size' => 20,
        '#maxlength' => 255,
      );
      $form['status_add']['weight'] = array(
        '#type' => 'weight',
        '#default_value' => 0,
        '#delta' => 15,
      );
      $form['submit'] = array(
        '#type' => 'button',
        '#value' => t('Save'),
      );

      return drupal_get_form('project_issue_admin_states_page', $form);
      break;
  }
}

function project_issue_admin_save_states() {
  // Check for and apply changes or additions to project issue status options.
  $edit = $_POST['edit'];
  if (isset($edit['default_state'])) {
    variable_set('project_issue_default_state', $edit['default_state']);
  }
  // Update existing status options.
  if($edit['status']) {
    foreach ($edit['status'] as $sid => $value) {
      $state = db_fetch_object(db_query('SELECT name, weight, author_has FROM {project_issue_state} WHERE sid = %d', $sid));
      // Check to see whether the record needs updating.
      if (($state->name != $value['name']) || ($state->weight != $value['weight']) || ($state->author_has != $value['author_has'])) {
        db_query("UPDATE {project_issue_state} SET name = '%s', weight = %d, author_has = %d WHERE sid = %d", $value['name'], $value['weight'], $value['author_has'], $sid);
      }
    }
  }
  // Add any new status options.
  if($edit['status_add']) {
    if($edit['status_add']['name']) {
      // Check to see whether the state already exists:
      $name = db_result(db_query("SELECT name FROM {project_issue_state} WHERE name = '%s'", $edit['status_add']['name']));
      if (!db_num_rows($name)) {
        db_query("INSERT INTO {project_issue_state} SET name = '%s', weight = %d, author_has = %d", $edit['status_add']['name'], $edit['status_add']['weight'], $edit['status_add']['author_has']);
      }
      else {
        drupal_set_message (t('Status %status already exists.', array ('%status' => $edit['status_add']['name'])));
      }
    }
  }
  drupal_goto('admin/settings/project/status');
}

function project_issue_admin_delete_state_confirm() {
  $sid = arg(5);
  $states = project_issue_state();
  $name = $states[$sid];

  $total = db_result(db_query('SELECT COUNT(nid) AS total FROM {project_issues} WHERE sid = %d', $sid));
  if ($total > 0) {
    $form['new_sid'] = array(
      '#type' => 'select',
      '#title' => t('Reassign status'),
      '#default_value' => $sid,
      '#options' => $states,
      '#description' => t('There are %total existing issues with the status of %name. Please select a new status for these issues.', array('%total' => $total, '%name' => $name)),
    );
  }
    $form['sid'] = array(
      '#type' => 'value',
      '#value' => $sid,
    );
    $form['name'] = array(
      '#type' => 'hidden',
      '#value' => $name,
    );
    return confirm_form(
      'project_issue_admin_delete_state',
      $form,
      t('Are you sure you want to delete the status option "%name"?', array('%name' => $name)),
      'admin/settings/project/status',
      t('This action cannot be undone.'),
      t('Delete'),
      t('Cancel')
    );
}

function project_issue_admin_delete_state_submit($form_id, $form_values) {
  if($form_values['new_sid'] == $form_values['sid']) {
      form_set_error('new_sid', t('Choose a new issue status for existing issues of status %name.', array('%name' => $form_values['name'])));
  } else {
    if ($form_values['new_sid']) {
      db_query('UPDATE {project_issues} SET sid = %d WHERE sid = %d', $form_values['new_sid'], $form_values['sid']);
    }
    db_query('DELETE FROM {project_issue_state} WHERE sid = %d', $form_values['sid']);
    drupal_set_message(t('Project issue status %issue deleted.', array('%issue' => $form_values['name'])));
    drupal_goto('admin/settings/project/status');
  }
}

function project_issue_query_result($query = NULL, $format = 'html', $search = true) {
  $query = project_issue_query_parse($query);

  $categories = array(0 => t('<all>')) + project_issue_category();
  $states = array(implode(',', array_keys(project_issue_state())) => t('<all>')) + project_issue_state();
  $priorities = array(0 => t('<all>')) + project_issue_priority();

  $projects = array(0 => t('<all>'));
  $result = db_query(db_rewrite_sql("SELECT n.nid, n.title FROM {node} n WHERE n.type = 'project_project' AND n.status = 1 AND n.moderate = 0 ORDER BY n.title"));
  while ($node = db_fetch_object($result)) {
    $projects[$node->nid] = $node->title;
  }

  // Load active project
  if (!$project) {
    if (count($query->projects) == 1) {
      $project = node_load($query->projects[0]);
    }
  }

  if ($project) {
    $releases = project_release_load($project, 0);
    $query->projects = $project->nid;
    $links = array(
      l(t('submit'), "node/add/project_issue/$project->uri", array('title' => t('Submit a new issue for %project.', array('%project' => $project->uri)))),
      l(t('statistics'), "project/issues/statistics/$project->uri", array('title' => t('See statistics about %project issues.', array('%project' => $project->uri)))),
      l(t('subscribe'), "project/issues/subscribe/$project->uri", array('title' => t('Receive email updates about issues for %project.', array('%project' => $project->uri)))),
    );
  }
  else {
    $links = array(
      l(t('submit'), "node/add/project_issue", array('title' => t('Submit a new issue.'))),
      l(t('statistics'), "project/issues/statistics", array('title' => t('See statistics about issues.'))),
      l(t('subscribe'), "project/issues/subscribe", array('title' => t('Receive email updates about issues.'))),
    );
  }

  $header = array();
  if (!$project->nid) {
    $header[] = array('data' => t('Project'), 'field' => 'p.pid');
  }
  $header[] = array('data' => t('Summary'), 'field' => 'n.title');
  $header[] = array('data' => t('Status'), 'field' => 'p.sid');
  $header[] = array('data' => t('Pri'), 'field' => 'p.priority');
  $header[] = array('data' => t('Category'), 'field' => 'p.category');
  if (count($releases)) {
    $header[] = array('data' => t('Version'), 'field' => 'p.rid');
  }
  $header[] = array('data' => t('Last updated'), 'field' => 'n.changed', 'sort' => 'desc');
  $header[] = array('data' => t('Assigned to'), 'field' => 'u.name');
  #$header[] = array('data' => t('submitted by'), 'field' => 'n.uid');

  if ($query) {
    $sql = project_issue_query_sql($query);
    $result = pager_query($sql['sql'] . tablesort_sql($header), 20, 0, $sql['count']);
  }
  else {
    $result = pager_query(db_rewrite_sql('SELECT p.nid FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid INNER JOIN {users} u ON p.assigned = u.uid INNER JOIN {node} np ON np.nid = p.pid WHERE n.status = 1 AND ('. implode(' OR ', $pids) .') AND (p.sid = 1 OR p.sid = 2)'. tablesort_sql($header), 'p'), 20, 0,
                          db_rewrite_sql('SELECT COUNT(p.nid) FROM {project_issues} p INNER JOIN {node} n ON n.nid = p.nid INNER JOIN {users} u ON p.assigned = u.uid INNER JOIN {node} np ON np.nid = p.pid WHERE n.status = 1 AND ('. implode(' OR ', $pids) .') AND (p.sid = 1 OR p.sid = 2)', 'p'));
  }

  if ($search) {
    // Action links:
    $group = theme('item_list', $links);

    // Convert array fields to single select form items. Some magic happens here.
    $fields = array('projects', 'states', 'priorities', 'categories', 'users');
    foreach ($fields as $field) {
      if (is_array($query->$field)) {
        $option = array();

        foreach ($query->$field as $value) {
          $option[] = ${$field}[$value];
        }
        $query->$field = implode(',', $query->$field);
        ${$field}[$query->$field] = implode(',', $option);
      }
    }

    // Make quick search form:
      $form['projects'] = array(
        '#type'=> 'select',
        '#title' => t('Project'),
        '#default_value' => $query->projects,
        '#options' => $projects,
      );
      $form['states'] = array(
        '#type'=> 'select',
        '#title' => t('Status'),
        '#default_value' => $query->states,
        '#options' => $states,
      );
      $form['categories'] = array(
        '#type'=> 'select',
        '#title' => t('Category'),
        '#default_value' => $query->categories,
        '#options' => $categories,
      );
      $form['priorities'] = array(
        '#type'=> 'select',
        '#title' => t('Priority'),
        '#default_value' => $query->priorities,
        '#options' => $priorities,
      );
      $form['users'] = array(
        '#type'=> 'hidden',
        '#value' => $query->users,
      );
      $form['submit'] = array(
        '#type' => 'button',
        '#value' => t('Search'),
      );
      $form['advanced_search'] = array(
        '#type' => 'markup',
        '#value'=> l(t('advanced search'), 'project/issues/search'. ($project->uri ? "/$project->uri" : "")),
      );
    $group .= drupal_get_form('project_issue_query_result_quick_search', $form);
  }

  $rows = array();

  if ($format == 'rss') {
    return project_issue_query_rss($projects, $result);
  }
  elseif (db_num_rows($result)) {
    while ($node = db_fetch_object($result)) {
      $node = node_load($node->nid);
      $row = array();
      $number++;
      $class = 'state_'. ($number % 2 ? 'light' : 'dark') ."_$node->sid";
      if (!$project->nid) {
        $row[] = l($projects[$node->pid], "project/issues/$node->pid");
      }
      $row[] = l(substr($node->title, 0, 50), "node/$node->nid") . theme('mark', node_mark($node->nid, $node->changed));
      $row[] = $states[$node->sid];
      $row[] = $priorities[$node->priority];
      $row[] = t($node->category);
      if (count($releases)) {
        $row[] = $releases[$node->rid];
      }
      $row[] = array('data' => format_interval(time() - $node->changed, 2), 'align' => 'right');
      $row[] = ($node->assigned) ? theme('username', user_load(array('uid' => $node->assigned))) : '';

      $row = array('data' => $row, 'class' => $class);

      $rows[] = $row;
    }
    $query = project_issue_query_pager($query);

    $rss = theme('xml_icon', url('project/issues/rss', project_issue_query_url($query)));
    $link = l(t('#'), 'project/issues', array('title' => t('Permalink to search query.')), project_issue_query_url($query));

    if ($pager = theme("pager", NULL, 20, 0, $query + tablesort_pager())) {
      $rows[] = array(array('data' => $pager, 'colspan' => count($header)));
    }
  }
  else {
    $rows[] = array(array('data' => t('No issues found.'), 'colspan'=> 8));
  }

  $output = '<div class="project">';
  $output .= '<div class="quick-search">';
  $output .= $group;
  $output .= '</div>';
  $output .= theme('table', $header, $rows);
  $output .= "$rss $link";
  $output .= '</div>';
  return $output;
}

function theme_project_issue_query_result_quick_search($form) {
  $rows[] = array(
    form_render($form['projects']),
    form_render($form['states']),
    form_render($form['categories']),
    form_render($form['priorities']),
    form_render($form['users']) . form_render($form['submit']),
    form_render($form['advanced_search']),
  );
  $output = theme('table', array(), $rows);
  $output .= form_render($form);
  return $output;
}

function project_issue_query_url($query = 0) {
  static $url = NULL;

  if (is_array($query) && $url === NULL) {
    foreach ($query as $key => $value) {
      $url[] = "$key=$value";
    }
    $url = implode('&', $url);
  }
  return $url;
}

function project_issue_query_parse($query = NULL) {
  $fields = array('projects', 'text', 'attachment', 'summary', 'comment', 'categories', 'components', 'versions', 'states', 'priorities', 'users', 'assigned', 'submitted', 'participated');
  if ($_SERVER['REQUEST_METHOD'] == 'POST' && is_array($_POST['edit'])) {
    foreach ($_POST['edit'] as $key => $value) {
      if (!empty($value) && in_array($key, $fields)) {
        $query->$key = !is_array($value) ? explode(',', $value) : $value;
        $_POST[$key] = is_array($value) ? implode(',', $value) : $value;
      }
    }
    unset($_POST['edit'], $_POST['op']);
  }
  else {
    foreach ($_GET as $key => $value) {
      if (!empty($value) && in_array($key, $fields)) {
        $query->$key = explode(',', $value);
      }
    }
  }

  if (empty($query->states)) {
    $query->states = array(1, 2, 8, 13, 14);
  }

  return $query;
}

function project_issue_query_sql_field($field, $values, $like = 0, $operator = ' OR ', $callback = 0) {
  $sql = array();
  if (!is_array($values)) {
    $values = array($values);
  }
  foreach ($values as $value) {
    $value = db_escape_string($value);
    if ($callback) {
      $value = $callback($value);
    }
    $sql[] = ($like && $field != 'p.pid') ? "$field LIKE '%$value%'" : "$field = '$value'";
  }
  if ($sql) {
    return '('. implode($operator, $sql) .')';
  }
}

function project_issue_query_pager($query) {
  $get = array();
  if (count($query)) {
    foreach ($query as $key => $value) {
      $get[$key] = (is_array($value)) ? implode(',', $value) : $value;
    }
  }
  return $get;
}

function project_issue_query_sql($query) {
  foreach ($query as $key => $value) {
    switch ($key) {
      case 'projects':
        $sql[] = project_issue_query_sql_field('p.pid', $value, 1);
        break;
      case 'text':
        $comments = 1;
        $sql[] = '('. project_issue_query_sql_field('n.title', $value, 1) .' OR '. project_issue_query_sql_field('r.body', $value, 1) .' OR '. project_issue_query_sql_field('c.body', $value, 1) .')';
        break;
      case 'attachment':
        $comments = 1;
        $sql[] = "(p.file_path <> '' OR c.file_path <> '')";
        break;
      case 'summary':
        $sql[] = '('. project_issue_query_sql_field('n.title', $value, 1) .' OR '. project_issue_query_sql_field('r.body', $value, 1) .')';
        break;
      case 'comment':
        $comments = 1;
        $sql[] = project_issue_query_sql_field('c.body', $value, 1);
        break;
      case 'categories':
        $sql[] = project_issue_query_sql_field('p.category', $value);
        break;
      case 'components':
        $sql[] = project_issue_query_sql_field('p.component', $value);
        break;
      case 'versions':
        $sql[] = project_issue_query_sql_field('p.rid', $value);
        break;
      case 'states':
        $sql[] = project_issue_query_sql_field('p.sid', $value);
        break;
      case 'priorities':
        $sql[] = project_issue_query_sql_field('p.priority', $value);
        break;
      case 'users':
        $_sql = array(
          project_issue_query_sql_field('n.uid', $value, 0, ' OR ', 'project_issue_query_user'),
          project_issue_query_sql_field('p.assigned', $value, 0, ' OR ', 'project_issue_query_user')
        );
        $sql[] = '('. implode(' OR ', $_sql) .')';
        break;
      case 'participated':
        $comments = 1;
        $_sql = array(
          project_issue_query_sql_field('n.uid', $value, 0, ' OR ', 'project_issue_query_user'),
          project_issue_query_sql_field('p.assigned', $value, 0, ' OR ', 'project_issue_query_user'),
          project_issue_query_sql_field('c.uid', $value, 0, ' OR ', 'project_issue_query_user')
        );
        $sql[] = '('. implode(' OR ', $_sql) .')';
        break;
      case 'assigned':
        $sql[] = project_issue_query_sql_field('p.assigned', $value, 0, ' OR ', 'project_issue_query_user');
        break;
      case 'submitted':
        $sql[] = project_issue_query_sql_field('n.uid', $value, 0, ' OR ', 'project_issue_query_user');
        break;
    }
  }

  if (!$comments) {
    return array(
      'sql' => db_rewrite_sql('SELECT n.nid FROM {node} n INNER JOIN {project_issues} p ON p.nid = n.nid INNER JOIN {node_revisions} r ON r.vid = n.vid INNER JOIN {users} u ON p.assigned = u.uid WHERE n.status = 1 AND ('. implode(') AND (', $sql) .')'),
      'count' => db_rewrite_sql('SELECT COUNT(n.nid) FROM {node} n INNER JOIN {project_issues} p ON p.nid = n.nid INNER JOIN {node_revisions} r ON r.vid = n.vid INNER JOIN {users} u ON p.assigned = u.uid WHERE n.status = 1 AND ('. implode(') AND (', $sql) .')')
    );
  }
  else {
    return array(
      'sql' => db_rewrite_sql('SELECT DISTINCT(n.nid) FROM {node} n INNER JOIN {project_issues} p ON p.nid = n.nid INNER JOIN {node_revisions} r ON r.vid = n.vid INNER JOIN {users} u ON p.assigned = u.uid LEFT JOIN {project_comments} c ON c.nid = p.nid WHERE n.status = 1 AND ('. implode(') AND (', $sql) .')'),
      'count' => db_rewrite_sql('SELECT COUNT(DISTINCT(n.nid)) FROM {node} n INNER JOIN  {project_issues} p ON p.nid = n.nid INNER JOIN {node_revisions} r ON r.vid = n.vid INNER JOIN {users} u ON p.assigned = u.uid LEFT JOIN {project_comments} c ON c.nid = p.nid WHERE n.status = 1 AND ('. implode(') AND (', $sql) .')')
    );
  }
}

function project_issue_query_user($value) {
  if (is_numeric($value)) {
    return $value;
  }
  else {
    $uid = db_result(db_query("SELECT uid FROM {users} WHERE name = '%s'", $value));
    if (!$uid) {
      drupal_set_message(t("Username '%user' not found.", array('%user' => $value)), 'error');
    }
    return $uid;
  }
}

function project_issue_query_rss($projects, $result) {
  global $base_url, $locale;

  if (db_num_rows($result)) {
    while ($node = db_fetch_object($result)) {
      $node = node_load($node->nid);

      $body = $node->teaser ? $node->teaser : $node->body;

      $items .= format_rss_item(
        $node->title,
        url("node/$node->nid", NULL, NULL, true),
        $body,
        array(
          'pubDate' => date('r', ($node->updated > $node->created) ? $node->updated : $node->created),
          'comments' => url("node/$node->nid", NULL, NULL, true),
          'category' => $projects[$node->pid]
        )
      );
    }
  }
  else {
    $rows[] = array(array('data' => t('No issues found.'), 'colspan'=> 8));
  }

  $title = variable_get('site_name', 'drupal') .' - '. t('issues');
  $link = url('project/issues', NULL, NULL, TRUE); // not 100% correct
  $description = variable_get('site_mission', '');

  $output .= "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $output .= "<rss version=\"0.92\" xml:base=\"". $base_url ."\">\n";
  $output .= format_rss_channel($title, $link, $description, $items, $locale);
  $output .= "</rss>\n";

  return $output;
}

function project_issue_tracker_comments($node) {
  return db_query(db_rewrite_sql("SELECT c.nid, c.cid, c.uid, c.created AS timestamp, '%s' AS subject, u.name FROM {project_comments} c INNER JOIN {users} u ON c.uid = u.uid WHERE c.nid = %d ORDER BY c.cid DESC", 'c'), $node->title, $node->nid);
}

?>
