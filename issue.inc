<?php
/* $Id: issue.inc,v 1.90 2005/02/06 12:32:41 dries Exp $ */

function project_issue_node_name() {
  return t('issue');
}

function project_issue_page() {
  switch ($_POST['op'] ? $_POST['op'] : arg(2)) {
    case 'search':
      $project = project_project_retrieve(arg(3));
      print theme('page', project_issue_query($project, $query));
      break;
    case 'rss':
      $project = project_project_retrieve(arg(3));
      $query = new StdClass();
      $query->projects = array($project->nid);

      header('Content-Type: text/xml');
      print project_issue_query_result($query, 'rss');
      break;
    case 'add':
      $project = project_project_retrieve(arg(3));
      drupal_goto("node/add/project_issue/$project->uri");
      break;
    case 'statistics':
      $project = project_project_retrieve(arg(3));
      print theme('page', project_issue_statistics($project));
      break;
    case t('Subscribe'):
    case 'subscribe':
      $project = project_project_retrieve(arg(3));
      if ($GLOBALS['user']->uid) {
        print theme('page', project_issue_subscribe($project));
        break;
      }
    case 'user':
       $_GET['users'] = $GLOBALS['user']->uid;
    default:
      $project = project_project_retrieve(arg(2));
      $query = new StdClass();
      $query->projects = array($project->nid);

      print theme('page', project_issue_query_result($query));
  }
}

function project_issue_statistics($project) {
  if ($project->nid) {
    $breadcrumb = array(
      l(t('Home'), NULL),
      l(t('Projects'), 'project')
    );
    if (function_exists('taxonomy_node_get_terms') && $term = reset(taxonomy_node_get_terms($project->nid))) {
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
  $result = db_query('SELECT p.category, SUM(n.changed - n.created) / COUNT(p.category) AS duration FROM {project_issues} p INNER JOIN {node} n USING (nid) WHERE n.status = 1%s AND p.state > 1 GROUP BY p.category', $filter);
  while ($stat = db_fetch_object($result)) {
    $i++;
    $rows[$i][0] = project_issue_category($stat->category);;
    $rows[$i][1] = array('data' => format_interval($stat->duration, 2), 'class' => 'numeric');
  }

  $result = db_query('SELECT p.category, SUM(n.changed - n.created) / COUNT(p.category) AS duration FROM {project_issues} p INNER JOIN {node} n USING (nid) WHERE n.status = 1%s AND p.state > 1 AND n.created > %d GROUP BY p.category', $filter, $duration);
  while ($stat = db_fetch_object($result)) {
    $j++;
    $rows[$j][2] = array('data' => format_interval($stat->duration, 2), 'class' => 'numeric');
  }
  $output .= '<h2>'. t('Average lifetime') .'</h2>';
  $output .= theme('table', $header, $rows);

  $header = array(t('Status'), t('Overall'), '%', t('Last month'));
  $rows = array();
  // Activity overall
  $total = db_result(db_query('SELECT COUNT(n.nid) AS total FROM {project_issues} p INNER JOIN {node} n USING (nid) WHERE n.status = 1%s', $filter));
  $result = db_query('SELECT p.state, COUNT(n.nid) AS total FROM {project_issues} p INNER JOIN {node} n USING (nid) WHERE n.status = 1%s GROUP BY p.state', $filter);
  while ($stat = db_fetch_object($result)) {
    $rows[$stat->state][0] = project_issue_state($stat->state);;
    $rows[$stat->state][1] = array('data' => $stat->total, 'class' => 'project-numeric');
    $rows[$stat->state][2] = array('data' => number_format($stat->total / $total * 100) .'%', 'class' => 'project-numeric2');
  }
  // Activity this week
  $result = db_query('SELECT p.state, COUNT(n.nid) AS total FROM {project_issues} p INNER JOIN {node} n USING (nid) WHERE n.status = 1%s AND n.changed > %d GROUP BY p.state', $filter, $duration);
  while ($stat = db_fetch_object($result)) {
    $rows[$stat->state][3] = array('data' => $stat->total, 'class' => 'project-numeric');
  }
  $output .= '<h2>'. t('Issue activity') .'</h2>';
  $output .= theme('table', $header, $rows);

  // Project overview
  if (!$project->nid) {
    $header = array_merge(array(t('Project')), project_issue_state(), array(t('Total')));
    $rows = array();
    $result = db_query("SELECT n.nid, n.title, p.state, COUNT(p.nid) AS total FROM {node} n INNER JOIN {project_issues} p ON n.nid = p.pid WHERE n.type = 'project_project' AND n.status = 1 GROUP BY n.nid, p.state");

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
      $rows[$stat->nid][$stat->state]['data'] = $stat->total;
    }
    usort($rows, create_function('$a,$b', 'return $a[1]["data"] < $b[1]["data"];'));
    $output .= '<h2>'. t('Project overview') .'</h2>';
    $output .= theme('table', $header, $rows);
  }

  $output .= '</div>';
  return $output;
}

function project_issue_subscribe($project = 0) {
  global $user;

  $levels = array(0 => t('None'), 1 => t('Own issues'), 2 => t('All issues'));

  if (is_array($_POST['edit'])) {
    // Remove previous subscriptions for user.
    if ($project->nid) {
      db_query('DELETE FROM {project_subscriptions} WHERE nid = %d AND uid = %d', $project->nid, $user->uid);
    }
    else {
      db_query('DELETE FROM {project_subscriptions} WHERE uid = %d', $user->uid);
    }

    if ($_POST['all']) {
      $_level = array_search($_POST['all'], $levels);
    }

    foreach ($_POST['edit'] as $nid => $level) {
      if ($_level !== 0 && $level !== 0) {
        db_query('INSERT INTO {project_subscriptions} (nid, uid, level) VALUES (%d, %d, %d)', $nid, $user->uid, $_level ? $_level : $level);
      }
    }
    drupal_set_message(t('Subscription settings saved.'));
  }

  if ($project) {
    $level = db_result(db_query('SELECT level FROM {project_subscriptions} WHERE nid = %d AND uid = %d', $project->nid, $user->uid));

    $output = '<p>'. t('Subscribe to receive e-mail notification when an issue for this project is updated.') .'</p>';

    $output .= form_radios(t('Subscribe to %project issues', array('%project' => $project->title)), "][$project->nid", $level, $levels);
  }
  else {
    $headers = array_merge(t('Project'), $levels);

    $row = array(t('All projects'));
    foreach ($levels as $key => $level) {
      $row[] = form_submit($level, 'all');
    }
    $rows = array($row);
    $nids = array();

    $result = db_query("SELECT n.nid, n.title, s.level FROM {project_subscriptions} s INNER JOIN {node} n USING (nid) WHERE n.type = 'project_project' AND n.status = 1 AND s.uid = %d ORDER BY n.title", $user->uid);
    while ($project = db_fetch_object($result)) {
      $row = array($project->title);
      foreach ($levels as $key => $level) {
        $row[] = form_radio($level, "$project->nid", $key, $project->level == $key);
      }
      $rows[] = $row;
      $nids[] = $project->nid;
    }

    $result = db_query("SELECT nid, title FROM {node} WHERE type = 'project_project' AND status = 1". ($nids ?  " AND nid NOT IN (%s)" : "") ." ORDER BY title", implode(',', $nids));
    while ($project = db_fetch_object($result)) {
      $row = array($project->title);
      foreach ($levels as $key => $level) {
        $row[] = form_radio($level, "$project->nid", $key, $key == 0);
      }
      $rows[] = $row;
    }
    $output = theme('table', $headers, $rows);
  }

  $output .= form_submit(t('Subscribe'));
  return form($output);
}

function project_issue_form(&$node, &$param) {
  global $user;

  // Set form parameters so we can accept file uploads.
  $param['options'] = array('enctype' => 'multipart/form-data');

  // Fetch a list of all projects to make swapping simpler
  $projects = array(t('<none>'));
  $result = db_query('SELECT n.nid, n.title FROM {project_projects} p INNER JOIN {node} n USING (nid) WHERE n.status = 1 AND p.issues = 1');
  while ($project = db_fetch_object($result)) {
    $projects[$project->nid] = $project->title;
  }

  if ($node->pid) {
    // Load the project and initialize some support arrays.
    $project = node_load(array('nid' => $node->pid, 'type' => 'project_project'));
    if (trim($project->help)) {
      drupal_set_message($project->help);
    }
    if ($releases = project_release_load($project)) {
      $releases = array(t('<none>')) + $releases;
    }
    if ($project->components) {
      $components = array(t('<none>'));
      foreach ($project->components as $component) {
        $components[$component] = $component;
      }
    }
    $categories = array_merge(t('<none>'), project_issue_category(0, 0));
    $priorities = project_issue_priority();
    $states = project_issue_state();

    if ($user->uid == $node->assigned) {
      $assigned = array(0 => t('Unassign'), $user->uid => $user->name);
    }
    else {
      $assigned = array(
        $node->assigned => ($node->assigned && ($account = user_load(array('uid' => $node->assigned))) ? $account->name : t('Unassigned')),
        $user->uid => $user->name
      );
    }

    $group1 = form_select(t('Project'), 'pid', $node->pid, $projects, NULL, 0, FALSE, TRUE);
    if ($releases) {
      $group1 .= form_select(t('Version'), 'rid', $node->rid, $releases, NULL, 0, FALSE, TRUE);
    }
    $group1 .= form_select(t('Component'), 'component', $node->component, $components, NULL, 0, FALSE, TRUE);

    $group2 = form_select(t('Category'), 'category', $node->category, $categories, NULL, 0, FALSE, TRUE);
    $group2 .= form_select(t('Priority'), 'priority', $node->priority, $priorities);
    $group2 .= form_select(t('Assigned'), 'assigned', $node->assigned, $assigned);
    $group2 .= form_select(t('Status'), 'state', $node->state, $states);

    $output .= '</div>';
    $output .= '<div class="admin">';
    $output .= '<div class="options">'. form_group(t('Project information'), $group1) .'</div>';
    $output .= '<div class="options">'. form_group(t('Issue information'), $group2) .'</div>';
    $output .= '</div>';
    $output .= '<div class="standard">';

    if (function_exists('taxonomy_node_form')) {
      $output .= implode('', taxonomy_node_form($node->type, $node));
    }

    $output .= form_textarea('Description', 'body', $node->body, 60, 20, NULL, NULL, !$node->comment);
    $output .= filter_form('format', $node->format);
    $output .= form_file(t('File attachment'), 'file_issue', 40, ($node->file || $node->file_path ? t('A file already exists, if you upload another file the current file will be replaced.') : t('Optionally attach a file, for example a patch or a screenshot.')));
  }
  else {
    $output = form_select(t('Project'), 'pid', $node->pid, $projects);
  }

  return $output;
}

function project_issue_validate(&$node) {
  // Try to find the active project
  if (empty($node->pid) && $arg = arg(3)) {
    if (is_numeric($arg)) {
      $node->pid = db_result(db_query('SELECT nid FROM {project_projects} WHERE nid = %d', $arg), 0);
    }
    else {
      $node->pid = db_result(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s'", $arg), 0);
    }
  }

  // Validate the rest of the form when we have a project.
  if (!$node->validated) {
    if ($node->pid && $project = node_load(array('nid' => $node->pid))) {
      if ($releases = project_release_load($project)) {
        if (!$node->rid || !$releases[$node->rid]) {
          $node->rid = $project->version;
        }
        empty($node->rid) and form_set_error('rid', t('You have to specify a valid version.'));
      }
      if ($node->component && !in_array($node->component, $project->components)) {
        $node->component = 0;
      }
      empty($node->priority) and $node->priority = 2;
      empty($node->category) and $node->category = arg(4);
      empty($node->state) and $node->state = 1;

      if (isset($node->title)) {
        empty($node->component) and form_set_error($error['component'], t('You have to specify a valid component.'));
        empty($node->category) and form_set_error('category', t('You have to specify a valid category.'));
        empty($node->body) and !$node->comment and form_set_error('body', t('You have to specify a valid description.'));
      }

      $file = file_check_upload('file_issue');
      $node->file = file_save_upload($file);
    }
    else {
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

  if (!$teaser) {
    $project = node_load(array('nid' => $node->pid, 'type' => 'project_project'));
    $release = project_release_load($node->rid);
    $assigned = ($node->assigned && ($account = user_load(array('uid' => $node->assigned))) ? $account->name : t('Unassigned'));

    $output = '<div class="project">';
    $rows = array();
    $rows[] = array('Project:', $project->title);
    if ($release->version) {
      $rows[] = array('Version:', $release->version);
    }
    $rows[] = array(t('Component:'),  drupal_specialchars($node->component));
    $rows[] = array(t('Category:'), drupal_specialchars($node->category));
    $rows[] = array(t('Priority:'), project_issue_priority($node->priority));
    $rows[] = array(t('Assigned:'), $assigned);
    $rows[] = array(t('Status:'), project_issue_state($node->state));
    if ($node->file_path && file_exists($node->file_path)) {
      $rows[] = array(t('Attachment:'), '<a href="'. file_create_url($node->file_path). '">'. basename($node->file_path) .'</a> ('. format_size($node->file_size) .')');
    }
    $output .= '<div class="summary">'. theme('table', NULL, $rows) .'</div>';

    $output .= '<p>'. t('Description') .'</p>';
    $output .= '<p>'. $node->body .'</p>';
    $output .= '</div>';

    $node->body = $output;

    // Breadcrumb navigation
    $breadcrumb[] = array('path' => 'project', 'title' => t('projects'));
    if (function_exists('taxonomy_node_get_terms') && $term = reset(taxonomy_node_get_terms($node->pid))) {
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
  $project = db_fetch_object(db_query('SELECT * FROM {project_issues} WHERE nid = %d', $node->nid));
  return $project;
}

function project_issue_insert($node) {
  if ($node->file) {
    $file = file_save_upload($node->file, variable_get('project_directory_issues', 'issues'));
  }

  db_query("INSERT INTO {project_issues} (nid, pid, category, component, priority, rid, assigned, state, file_path, file_mime, file_size) VALUES (%d, %d, '%s', '%s', %d, %d, %d, %d, '%s', '%s', %d)", $node->nid, $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->state, $file->filepath, $file->filemime, $file->filesize);
  project_mail_notify($node);
}

function project_issue_update($node) {
  if ($node->file) {
    // Remove old file.
    file_delete(db_result(db_query('SELECT file_path FROM {project_issues} WHERE nid = %d', $node->nid)));
    $file = file_save_upload($node->file, variable_get('project_directory_issues', 'issues'));
    db_query("UPDATE {project_issues} SET pid = %d, category = '%s', component = '%s', priority = %d, rid = %d, assigned = %d, state = %d, file_path = '%s', file_mime = '%s', file_size = %d WHERE nid = %d", $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->state, $file->filepath, $file->filemime, $file->filesize, $node->nid);
  }
  else {
    db_query("UPDATE {project_issues} SET pid = %d, category = '%s', component = '%s', priority = %d, rid = %d, assigned = %d, state = %d WHERE nid = %d", $node->pid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->state, $node->nid);
  }
  project_mail_notify($node);
}

function project_issue_delete($node) {
  file_delete($node->file_path);
  db_query('DELETE FROM {project_issues} WHERE nid = %d', $node->nid);
}

function project_issue_access($op, $node) {
  switch ($op) {
    case 'view':
      return user_access('access project issues');
    case 'create':
      return user_access('create project issues');
    case 'update':
    case 'delete':
      return user_access('administer projects');
  }
}

// Support stuff
function project_issue_state($state = 0) {
  $states = array(1 => t('active'), 2 => t('fixed'), 8=> t('patch'), 3 => t('duplicate'), 4 => t('postponed'), 5 => t("won't fix"), 6 => t('by design'), 7 => t('closed'));
  return $state ? $states[$state] : $states;
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
  $result = db_query('SELECT p.state, count(p.state) AS count FROM {node} n INNER JOIN {project_issues} p USING (nid) WHERE n.status = 1 AND p.pid = %d GROUP BY p.state', $pid);
  while ($data = db_fetch_object($result)) {
    $state[$data->state] = $data->count;
  }
  return $state;
}

function project_issue_query($project, $query) {
  $categories = project_issue_category();
  if ($project->nid) {
    foreach ($project->components as $component) {
      $components[$component] = $component;
    }
    $versions = project_release_load($project, 0);
  }
  else {
    $result = db_query("SELECT nid, title FROM {node} WHERE type = 'project_project' AND status = 1 AND moderate = 0 ORDER BY title");
    while ($node = db_fetch_object($result)) {
      $projects[$node->nid] = $node->title;
    }
  }

  $states = project_issue_state();
  $priorities = project_issue_priority();

  $rows[] = array(
    array('data' => form_textfield(t('Search for'), 'text', $query->summary, 60, 255), 'colspan' => 3)
  );
  $rows[] = array(
    array('data' => form_checkbox(t('Has attachment.'), 'attachment', 1, $query->attachment), 'colspan' => 3)
  );
  if ($projects) {
    $rows[] = array(
      form_select(t('Projects'), 'projects', $query->projects, $projects, '', 'size="5"', 1),
      form_select(t('Categories'), 'categories', $query->categories, $categories, '', 'size="5"', 1),
    );
  }
  else {
    $rows[] = array(
      form_select(t('Versions'), 'versions', $query->versions, $versions, '', 'size="5"', 1),
      form_select(t('Components'), 'components', $query->components, $components, '', 'size="5"', 1),
      form_select(t('Categories'), 'categories', $query->categories, $categories, '', 'size="5"', 1),
    );
  }
  $rows[] = array(
    form_select(t('Priorities'), 'priorities', $query->priorities, $priorities, '', 'size="5"', 1),
    form_select(t('Status'), 'states', ($query->states) ? $query->states : array(1, 2, 8), $states, '', 'size="5"', 1)
  );
  $rows[] = array(
    array('data' => form_textfield(t('Submitted by'), 'submitted', $query->submitted, 20, 255)),
    array('data' => form_textfield(t('Assigned'), 'assigned', $query->assigned, 20, 255)),
    array('data' => form_textfield(t('Participant'), 'participated', $query->users, 20, 255))
  );
  $rows[] = array(
    array('data' => form_submit(t('Search')), 'colspan' => 3)
  );

  $output = '<div class="project">';
  $output .= form(theme('table', NULL, $rows), 'post', url($project->uri ? "project/issues/$project->uri" : 'project/issues'));
  $output .= '</div>';

  return $output;
}

function project_issue_query_result($query = NULL, $format = 'html', $search = true) {
  $query = project_issue_query_parse($query);
  
  $categories = array(0 => t('<all>')) + project_issue_category();
  $states = array(implode(',', array_keys(project_issue_state())) => t('<all>')) + project_issue_state();
  $priorities = array(0 => t('<all>')) + project_issue_priority();

  $projects = array(0 => t('<all>'));
  $result = db_query("SELECT nid, title FROM {node} WHERE type = 'project_project' AND status = 1 AND moderate = 0 ORDER BY title");
  while ($node = db_fetch_object($result)) {
    $projects[$node->nid] = $node->title;
  }

  // Load active project
  if (!$project) {
    if (count($query->projects) == 1) {
      $project = node_load(array('nid' => $query->projects[0]));
    }
  }

  if ($project) {
    $releases = project_release_load($project, 0);
    $query->projects = $project->nid;
    $links = array(
      l(t('submit'), "node/add/project_issue/$project->uri"),
      l(t('statistics'), "project/issues/statistics/$project->uri"),
    );
  }
  else {
    $links = array(
      l(t('submit'), "node/add/project_issue"),
      l(t('statistics'), "project/issues/statistics"),
    );
  }

  $header = array();
  if (!$project->nid) {
    $header[] = array('data' => t('Project'), 'field' => 'p.pid');
  }
  $header[] = array('data' => t('Summary'), 'field' => 'n.title');
  $header[] = array('data' => t('Status'), 'field' => 'p.state');
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
    $result = pager_query($sql['sql'] . tablesort_sql($header), 20 , 0, $sql['count']);
  }
  else {
    $result = pager_query('SELECT n.nid FROM {project_issues} p INNER JOIN {node} n USING (nid) INNER JOIN {users} u ON p.assigned = u.uid INNER JOIN {node} np ON np.nid = p.pid WHERE n.status = 1 AND ('. implode(' OR ', $pids) .') AND (p.state = 1 OR p.state = 2)'. tablesort_sql($header), 20);
  }

  if ($search) {
    // Action links:
    $form = theme('item_list', $links);

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
    $rows = array(array(
      form_select(t('Project'), 'projects', $query->projects, $projects),
      form_select(t('Status'), 'states', $query->states, $states),
      form_select(t('Priority'), 'priorities', $query->priorities, $priorities),
      form_select(t('Category'), 'categories', $query->categories, $categories),
      form_hidden('users', $query->users) . form_submit(t('Search')),
      l(t('advanced search'), 'project/issues/search'. ($project->uri ? "/$project->uri" : ""))
    ));
    $form .= form(theme('table', '', $rows));
  }

  $rows = array();

  if ($format == 'rss') {
    return project_issue_query_rss($projects, $result);
  }
  elseif (db_num_rows($result)) {
    while ($node = db_fetch_object($result)) {
      $node = node_load($node);
      $row = array();
      $number++;
      $class = 'state_'. ($number % 2 ? 'light' : 'dark') ."_$node->state";
      if (!$project->nid) {
        $row[] = l($projects[$node->pid], "project/issues/$node->pid");
      }
      $row[] = l(substr($node->title, 0, 50), "node/$node->nid") .  theme('mark', node_mark($node->nid, $node->changed));
      $row[] = $states[$node->state];
      $row[] = $priorities[$node->priority];
      $row[] = t($node->category);
      if (count($releases)) {
        $row[] = $releases[$node->rid];
      }
      $row[] = array('data' => format_interval(time() - $node->changed, 2), 'align' => 'right');
      $row[] = ($node->assigned) ? format_name(user_load(array('uid' => $node->assigned))) : '';

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
  $output .= $form;
  $output .= '</div>';
  $output .= theme('table', $header, $rows);
  $output .= "$rss $link";
  $output .= '</div>';
  return $output;
}

function project_issue_query_url($query = 0) {
  static $url = NULL;

  if (is_array($query) && $url === NULL) {
    foreach ($query as $key => $value) {
      $url[] = "$key=$value";
    }
    $url = implode('&amp;', $url);
  }
  return $url;
}

function project_issue_query_parse($query = NULL) {
  $fields = array('projects', 'text', 'attachment', 'summary', 'comment', 'categories', 'components', 'versions', 'states', 'priorities', 'users', 'assigned', 'submitted');
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
    $query->states = array(1, 2, 8);
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
    $sql[] = $like ? "$field LIKE '%$value%'" : "$field = '$value'";
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
        $sql[] = '('. project_issue_query_sql_field('n.title', $value, 1) .' OR '. project_issue_query_sql_field('n.body', $value, 1) .' OR '. project_issue_query_sql_field('c.body', $value, 1) .')';
        break;
      case 'attachment':
        $comments = 1;
        $sql[] = "(p.file_path <> '' OR c.file_path <> '')";
        break;
      case 'summary':
        $sql[] = '('. project_issue_query_sql_field('n.title', $value, 1) .' OR '. project_issue_query_sql_field('n.body', $value, 1) .')';
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
        $sql[] = project_issue_query_sql_field('p.state', $value);
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
    return array('sql' => 'SELECT n.nid FROM {project_issues} p INNER JOIN {node} n ON p.nid = n.nid INNER JOIN {users} u ON p.assigned = u.uid WHERE n.status = 1 AND ('. implode(' AND ', $sql) .')');
  }
  else {
    return array(
      'sql' => 'SELECT DISTINCT(n.nid) FROM {project_issues} p INNER JOIN {node} n ON p.nid = n.nid INNER JOIN {users} u ON p.assigned = u.uid LEFT JOIN {project_comments} c ON c.nid = p.nid WHERE n.status = 1 AND ('. implode(' AND ', $sql) .')',
      'count' => 'SELECT COUNT(DISTINCT(n.nid)) FROM {project_issues} p INNER JOIN {node} n ON p.nid = n.nid INNER JOIN {users} u ON p.assigned = u.uid LEFT JOIN {project_comments} c ON c.nid = p.nid WHERE n.status = 1 AND ('. implode(' AND ', $sql) .')'
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
      $node = node_load($node);

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
  $output .= "<!DOCTYPE rss [<!ENTITY % HTMLlat1 PUBLIC \"-//W3C//ENTITIES Latin 1 for XHTML//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent\">]>\n";
  $output .= "<rss version=\"0.92\" xml:base=\"". $base_url ."\">\n";
  $output .= format_rss_channel($title, $link, $description, $items, $locale);
  $output .= "</rss>\n";
  
  return $output;
}

function project_issue_tracker_comments($node) {
  return db_query("SELECT c.cid, c.nid, c.uid, c.created AS timestamp, '%s' AS subject, u.name FROM {project_comments} c INNER JOIN {users} u ON c.uid = u.uid WHERE c.nid = %d ORDER BY c.cid DESC", $node->title, $node->nid);
}

?>
