<?php
// $Id: project.inc,v 1.24 2004/01/26 20:52:48 kjartan Exp $

function project_project_node($field) {
  $info['name'] = t('project');

  return $info[$field];
}

function project_project_menu(&$node) {
  $categories = project_issue_category(0, 1);
  $uri = "project/$node->uri";

  menu($uri, strtolower($node->title), 'project_page', 3);
  if ($GLOBALS['user']->uid) {
    menu("$uri/issues/user", t('my issues'), 'project_page', 0);
    menu("$uri/issues/subscribe", t('subscribe'), 'project_page', 3);
  }
  foreach ($categories as $cat => $category) {
    menu("$uri/issues/$cat", $category, 'project_page', 2);
  }
  menu("$uri/issues/add", t('submit'), 'project_page', 3);
  menu("$uri/issues/statistics", t('statistics'), 'project_page', 4);
  menu("$uri/issues/search", t('search'), 'project_page', 5);
}

function project_project_form(&$node, &$error) {
  if (count($node->components)) {
    $node->components = implode("\n", $node->components);
  }
  else {
    $node->components = "Code\nDocumentation\nMiscellaneous\nUser interface";
  }

  /*
  ** Allow the following fields to be initialized via $_GET
  */
  foreach (array('uri', 'mail', 'homepage', 'issues', 'mail_reminder', 'mail_digest') as $field) {
    if ($_GET['edit'][$field]) {
      $node->$field = $_GET['edit'][$field];
    }
  }

  //$output .= form_textfield(t('Full project name'), 'title', $node->title, 40, 255, t('This is the publicly visible project name.'). theme('error', $error['title']));
  $project .= form_textarea(t('Full description'), 'body', $node->body, 40, 10, theme('error', $error['body']));
  $project .= form_textfield(t('Short project name'), 'uri', $node->uri, 40, 50, t('This will be used to generate a /projects/&lt;shortname&gt;/ URL for your project.'). theme('error', $error['uri']));
  if (variable_get('project_release_directory', '') && user_access('maintain projects')) {
    $project .= form_textfield(t('Release directory'), 'release_directory', $node->release_directory, 40, 255, theme('error', $error['release_directory']));
  }
  $project .= form_textfield(t('Project e-mail'), 'mail', $node->mail, 20, 255, t('E-mail address where the project owners can be contacted.'). theme('error', $error['mail']));
  $project .= form_textfield(t('Homepage'), 'homepage', $node->homepage, 40, 255, t('Link to project homepage.'). theme('error', $error['homepage']));
  $project .= form_textfield(t('Screenshots'), 'screenshots', $node->screenshots, 40, 255, t('Link to project screenshots.'). theme('error', $error['screenshots']));
  $project .= form_textfield(t('Changelog'), 'changelog', $node->changelog, 40, 255, t('Link to changelog.'). theme('error', $error['changelog']));
  $project .= form_textfield(t('CVS tree'), 'cvs', $node->cvs, 40, 255, t('Link to webcvs/viewcvs.'). theme('error', $error['cvs']));
  $project .= form_textfield(t('Demo site'), 'demo', $node->demo, 40, 255, t('Link to a live demo.'). theme('error', $error['demo']));

  $issue .= form_item(t('Issue tracker'), form_checkbox(t('Enable issue tracker'), 'issues', 1, $node->issues, t('Let users submit bug requests, patches, feature requests, support requests, etc.')));
  $issue .= form_select(t('Default version'), 'version', $node->version, project_release_load($node), t('Default version for issues.'));
  $issue .= form_textarea(t('Components'), 'components', $node->components, 20, 5);
  $issue .= form_textarea(t('Submission guidelines'), 'help', $node->help, 20, 5);

  $email .= form_textfield(t('Weekly critical issues report'), 'mail_digest', $node->mail_digest, 20, 255, t('To get a weekly digest of critical issues specify an e-mail address.') . theme('error', $error['mail_digest']));
  $email .= form_textfield(t('Issues e-mail address'), 'mail_copy', $node->mail_copy, 20, 255, t('If you wish to receive a copy of all the issues to a central location specify an address here.') . theme('error', $error['mail_copy']));
  $options = project_issue_category();
  foreach ($options as $key => $choice) {
    $data .= form_checkbox($choice, "mail_copy_filter][$key", $key, ($key == $node->mail_copy_filter[$key]));
  }
  $email .= form_item(t('Categories'), $data, t('Which issue categories to e-mail. If none are checked all categories will be posted.'));
  $email .= form_item(t('Monthly reminder'), form_checkbox(t('Send monthly reminders to users'), 'mail_reminder', 1, $node->mail_reminder, t('Enabling this will send a monthly reminder to users that have open issues registered.')));

  if (function_exists('taxonomy_node_form')) {
    $output = implode('', taxonomy_node_form('project', $node));
  }
  $output .= '<div class="project">';
  $output .= form_group(t('Project information'), $project);
  $output .= form_group(t('Issue information'), $issue);
  $output .= form_group(t('E-mail options'), $email);
  $output .= '</div>';
  return $output;
}

function project_project_cleanup($input) {
  return $input;
}

function project_project_validate(&$node) {
  $node->promote = 0;
  $node->moderate = 0;

  // Make sure title isn't already in use
  if (db_num_rows(db_query("SELECT nid FROM {node} WHERE type = '%s' AND status = 1 AND title = '%s' AND nid <> %d", $node->type, $node->title, $node->nid))) {
    $error['title'] .= t('This project name is already in use.');
  }

  // Validate uri.
  if (empty($node->uri)) {
    $error['uri'] = t('Required.');
  }
  else {
    // Make sure uri only includes valid characters
    if (!preg_match('/^[a-zA-Z0-9_]+$/', $node->uri)) {
      $error['uri'] = t('Please only use alphanumerical characters.');
    }

    // Make sure uri isn't already in use, or reserved
    if (in_array($node->uri, array('issues', 'releases')) || db_num_rows(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s' AND nid <> %d", $node->uri, $node->nid))) {
      $error['uri'] .= t('This project name is already in use.');
    }
  }
  if (!$error['uri']) {
    $node->path = "project/$node->uri";
  }

  // We need a description.
  if (empty($node->body)) {
    $error['body'] = t('Required.');
  }

  // Validate emails
  if (empty($node->mail)) {
    $error['mail'] = t('Required.');
  }
  elseif ($data = user_validate_mail($node->mail)) {
    $error['mail'] = $data;
  }

  if (!empty($node->mail_digest) && ($data = user_validate_mail($node->mail_digest))) {
    $error['mail_digest'] = $data;
  }

  if (!empty($node->mail_copy) && ($data = user_validate_mail($node->mail_copy))) {
    $error['mail_copy'] = $data;
  }

  if (is_array($node->mail_copy_filter)) {
    $node->mail_copy_filter = array_filter($node->mail_copy_filter, 'project_project_cleanup');
  }

  // Make sure all URL fields actually contain URLs.
  foreach (array('homepage', 'changelog', 'cvs', 'demo') as $uri) {
    if ($node->$uri && !preg_match('/^(http|https|ftp):\/\//i', $node->$uri)) {
      $error[$uri] = t('Invalid URL.');
    }
  }

  // Transform components into array.
  if (!is_array($node->components)) {
    if ($node->components) {
      $node->components = explode("\n", str_replace("\r", '', $node->components));
    }
    else {
      $node->components = array();
    }
  }

  return $error;
}

function project_project_view($node, $main = 0, $page = 0) {
  if (!$main) {
    $breadcrumb[] = l(t('Home'), NULL);
    $breadcrumb[] = l(t('Projects'), 'project');
    if (function_exists('taxonomy_node_get_terms') && $term = reset(taxonomy_node_get_terms($node->nid))) {
      $breadcrumb[] = l($term->name, 'project', NULL, "tid=$term->tid");
    }
    drupal_set_breadcrumb($breadcrumb);
    menu_set_active_item("project/$node->uri");

    foreach (array('homepage' => t('homepage'), 'changelog' => t('changelog'), 'cvs' => t('cvs tree'), 'demo' => t('demo site'), 'screenshots' => t('screenshots')) as $uri => $name) {
      if (!empty($node->$uri)) {
        $links[] = sprintf('<a href="%s">%s</a>', $node->$uri, $name);
      }
    }

    if ($links) {
      $node->body .= theme('item_list', $links, t('Links'));
    }

    if ($output = project_release_view($node)) {
      $node->body .= '<h1>'. t('Releases') .'</h1>';
      $node->body .= $output;
    }
  }

  return theme('node', $node, $main, $page);
}

function project_project_load($node) {
  $node = db_fetch_object(db_query('SELECT * FROM {project_projects} WHERE nid = %d', $node->nid));

  $node->components = unserialize($node->components);
  $node->mail_copy_filter = unserialize($node->mail_copy_filter);

  return $node;
}

function project_project_insert($node) {
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);

  db_query("INSERT INTO {project_projects} (nid, uri, homepage, changelog, cvs, demo, release_directory, issues, components, mail, mail_digest, mail_reminder, mail_copy, mail_copy_filter, version, help, screenshots) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', %d, '%s', '%s')", $node->nid, $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->reminder, $node->mail_copy, $node->mail_copy_filter, $node->version, $node->help, $node->screenshots);
}

function project_project_update($node) {
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);

  db_query("UPDATE {project_projects} SET uri = '%s', homepage = '%s', changelog = '%s', cvs = '%s', demo = '%s', release_directory = '%s', issues = %d, components = '%s', mail = '%s', mail_digest = '%s', mail_reminder = %d, mail_copy = '%s', mail_copy_filter = '%s', version = %d, help = '%s', screenshots = '%s' WHERE nid = %d", $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->mail_reminder, $node->mail_copy, $node->mail_copy_filter, $node->version, $node->help, $node->screenshots, $node->nid);
}

function project_project_delete($node) {
  db_query('DELETE FROM {project_projects} WHERE nid = %d', $node->nid);
}

function project_project_access($op, $node) {
  global $user;

  switch ($op) {
    case 'view':
      return user_access('access projects') && $node->status;
    case 'create':
      return user_access('maintain project') && $user->uid;
    case 'update':
    case 'delete':
      return $user->uid && ($user->uid == $node->uid || user_access('administer projects'));
  }
}

?>
