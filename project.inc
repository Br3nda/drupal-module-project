<?php
/* $Id: project.inc,v 1.46 2005/02/06 19:59:02 dries Exp $ */

function project_project_node_name() {
  return t('project');

  return $info[$field];
}

function project_project_form(&$node) {
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

  //$output .= form_textfield(t('Full project name'), 'title', $node->title, 40, 255, t('This is the publicly visible project name.'));
  $project .= form_textarea(t('Full description'), 'body', $node->body, 40, 10, null, null, true);
  $project .= filter_form('format', $node->format);
  $project .= form_textfield(t('Short project name'), 'uri', $node->uri, 40, 50, t('This will be used to generate a /project/&lt;shortname&gt;/ URL for your project.'), null, true);
  $project .= form_textfield(t('Project e-mail'), 'mail', $node->mail, 20, 255, t('E-mail address where the project owners can be contacted.'), null, true);
  $project .= form_textfield(t('Homepage'), 'homepage', $node->homepage, 40, 255, t('Link to project homepage.'));
  $project .= form_textfield(t('Documentation'), 'documentation', $node->documentation, 40, 255, t('Link to project documentation.'));
  $project .= form_textfield(t('License'), 'license', $node->license, 40, 255, t('Link to project license.'));
  $project .= form_textfield(t('Screenshots'), 'screenshots', $node->screenshots, 40, 255, t('Link to project screenshots.'));
  $project .= form_textfield(t('Changelog'), 'changelog', $node->changelog, 40, 255, t('Link to changelog.'));
  $project .= form_textfield(t('CVS tree'), 'cvs', $node->cvs, 40, 255, t('Link to webcvs/viewcvs.'));
  $project .= form_textfield(t('Demo site'), 'demo', $node->demo, 40, 255, t('Link to a live demo.'));

  $issue .= form_item(t('Issue tracker'), form_checkbox(t('Enable issue tracker'), 'issues', 1, $node->issues, t('Let users submit bug requests, patches, feature requests, support requests, etc.')));
  if ($releases = project_release_load($node)) {
    $issue .= form_select(t('Default version'), 'version', $node->version, $releases, t('Default version for issues.'));
  }
  $issue .= form_textarea(t('Components'), 'components', $node->components, 20, 5);
  $issue .= form_textarea(t('Submission guidelines'), 'help', $node->help, 20, 5);

  $email .= form_textfield(t('Weekly critical issues report'), 'mail_digest', $node->mail_digest, 20, 255, t('To get a weekly digest of critical issues specify an e-mail address.'));
  $email .= form_textfield(t('Issues e-mail address'), 'mail_copy', $node->mail_copy, 20, 255, t('If you wish to receive a copy of all the issues to a central location specify an address here.'));
  if (user_access('administer projects')) {
    $options = project_issue_category();
    foreach ($options as $key => $choice) {
      $data .= form_checkbox($choice, "mail_copy_filter][$key", $key, ($key == $node->mail_copy_filter[$key]));
    }
    $email .= form_item(t('Categories'), $data, t('Which issue categories to e-mail. If none are checked all categories will be posted.'));
    $options = project_issue_state();
    $filter_state = '';
    foreach ($options as $key => $choice) {
      $filter_state .= form_checkbox($choice, "mail_copy_filter_state][$key", $key, ($key == $node->mail_copy_filter_state[$key]));
    }
    $email .= form_item(t('States'), $filter_state, t('Which issue states to e-mail. If none are checked all states will be posted.'));
    $email .= form_item(t('Monthly reminder'), form_checkbox(t('Send monthly reminders to users'), 'mail_reminder', 1, $node->mail_reminder, t('Enabling this will send a monthly reminder to users that have open issues registered.')));
  }

  if (function_exists('taxonomy_node_form')) {
    $output = implode('', taxonomy_node_form($node->type, $node));
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
  if (!$error['uri']) {
    $node->path = "project/$node->uri";
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

  if (is_array($node->mail_copy_filter)) {
    $node->mail_copy_filter = array_filter($node->mail_copy_filter, 'project_project_cleanup');
  }

  if (is_array($node->mail_copy_filter_state)) {
    $node->mail_copy_filter_state = array_filter($node->mail_copy_filter_state, 'project_project_cleanup');
  }

  // Make sure all URL fields actually contain URLs.
  foreach (array('homepage', 'changelog', 'cvs', 'demo') as $uri) {
    if ($node->$uri && !preg_match('/^(http|https|ftp):\/\//i', $node->$uri)) {
      form_set_error($uri, t('%field is not a valid URL.', array('%field' => t(ucfirst($uri)))));
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

  return $node;
}

/**
 * Implementation of hook_content().
 */
function project_project_content($node, $teaser = false) {
  return node_prepare($node, $teaser);
}

function project_project_set_location(&$node, $extra = array()) {
  $breadcrumb = array();

  $breadcrumb[] = array('path' => 'project', 'title' => t('projects'));
  if (function_exists('taxonomy_node_get_terms') && $term = reset(taxonomy_node_get_terms($node->nid))) {
    $breadcrumb[] = array('path' => 'project/'. $term->name, 'title' => $term->name);
  }
  $breadcrumb = array_merge($breadcrumb, $extra);

  return $breadcrumb;
}

function project_project_view(&$node, $teaser = false, $page = false) {
  $node = project_project_content($node, $teaser);

  if ($page) {
    // Breadcrumb navigation
    $breadcrumb[] = array('path' => 'node/'. $node->nid, 'title' => $node->title);
    $breadcrumb = project_project_set_location($node, $breadcrumb);
    menu_set_location($breadcrumb);
    
    if ($releases = project_release_load($node)) {
      list($rid, $version) = each($releases);
      $release = project_release_load($rid);

      // Download section
      $links = array();
      
      if ($release->version == 'cvs') {
        $links[] = l(t('Download latest release (%version)', array('%version' => $release->version)), $release->path);
      }
      else {
        $links[] = l(t('Download latest release (%version, %date)', array('%version' => $release->version, '%date' => format_date($release->created, 'small'))), $release->path);
      }
      if ($release->changes) {
        $links[] = l(t('Read release notes'), 'node/'. $node->nid .'/release', null, null, 'version-'. $release->version);
      }
      if (count($releases) > 1) {
        $links[] = l(t('View older releases'), 'node/'. $node->nid .'/release');
      }
      if (node_access('update', $node)) {
        $links[] = l(t('Add new release'), 'node/'. $node->nid .'/release/add');
      }
      if ($links) {
        $output .= theme('item_list', $links, t('Releases'));
      }

      // Misc section
      $links = array();

      foreach (array('documentation' => t('Read documentation'), 'license' => t('Read license'), 'changelog' => t('Read complete log of changes'), 'demo' => t('Try out a demonstration'), 'screenshots' => t('Look at screenshots')) as $uri => $name) {
        if (!empty($node->$uri)) {
          $links[] = sprintf('<a href="%s">%s</a>', $node->$uri, $name);
        }
      }

      if ($links) {
        $output .= theme('item_list', $links, t('Resources'));
      }

      // Support section
      $links = array();

      $links[] = l(t('Support forum'), variable_get('project_support_forum', 'forum/18'));

      if ($node->issues) {
        #$output .= '<p>'. t('If you think you have found a bug please check the list of existing bugs reported, if you do not find it please submit a new bug report. Remember to provide as much information as possible to make it easier for the developers to fix the issue.') .'</p>';
        $links[] = l(t('View pending bug reports'), 'project/issues/'. $node->uri, null, 'categories=bug', null);
        $links[] = l(t('View pending feature requests'), 'project/issues/'. $node->uri, null, 'categories=feature', null);
        $links[] = l(t('Report new bug'), 'node/add/project_issue/'. $node->uri .'/bug');
        $links[] = l(t('Request new feature'), 'node/add/project_issue/'. $node->uri .'/feature');
      }

      if ($links) {
        $output .= theme('item_list', $links, t('Support'));
      }

      // Developer section
      $links = array();

      if ($node->issues) {
        #$output .= '<p>'. t('If you think you have found a bug please check the list of existing bugs reported, if you do not find it please submit a new bug report. Remember to provide as much information as possible to make it easier for the developers to fix the issue.') .'</p>';
        $links[] = l(t('View pending patches'), 'project/issues/'. $node->uri, null, 'states=8', null);
        $links[] = l(t('View available tasks'), 'project/issues/'. $node->uri, null, 'categories=task', null);
      }

      if ($node->cvs) {
        $links[] = sprintf('<a href="%s">%s</a>', $node->cvs, t('Browse the CVS repository'));
      }
      
      if (module_exist('cvs')) {
        $links[] = l(t('View CVS messages'), 'project/cvs/'. $node->nid);
        $links[] = l(t('Developers'), 'project/developers/'. $node->nid);
      }
      
      if ($links) {
        $output .= theme('item_list', $links, t('Development'));
      }
   }

    $node->body .= $output;
  }
}

function project_project_releases() {
  $node = node_load(array('nid' => arg(1)));

  // Breadcrumb navigation
  $breadcrumb[] = array('path' => 'node/'. $node->nid .'/', 'title' => $node->title);
  $breadcrumb[] = array('path' => 'node/'. arg(1) .'/release', 'title' => t('Releases'));
  $breadcrumb = project_project_set_location($node, $breadcrumb);

  menu_set_location($breadcrumb);

  $output = project_release_list($node);

  print theme('page', $output, $node->title);
}

function project_project_load($node) {
  $node = db_fetch_object(db_query('SELECT * FROM {project_projects} WHERE nid = %d', $node->nid));

  $node->components = unserialize($node->components);
  $node->mail_copy_filter = unserialize($node->mail_copy_filter);
  $node->mail_copy_filter_state = unserialize($node->mail_copy_filter_state);

  return $node;
}

function project_project_insert($node) {
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);
  $node->mail_copy_filter_state = serialize($node->mail_copy_filter_state);

  db_query("INSERT INTO {project_projects} (nid, uri, homepage, changelog, cvs, demo, release_directory, issues, components, mail, mail_digest, mail_reminder, mail_copy, mail_copy_filter, mail_copy_filter_state, version, help, screenshots, documentation, license) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s')", $node->nid, $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->reminder, $node->mail_copy, $node->mail_copy_filter, $node->mail_copy_filter_state, $node->version, $node->help, $node->screenshots, $node->documentation, $node->license);
  project_release_scan_directory($node->uri);
}

function project_project_update($node) {
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);
  $node->mail_copy_filter_state = serialize($node->mail_copy_filter_state);

  db_query("UPDATE {project_projects} SET uri = '%s', homepage = '%s', changelog = '%s', cvs = '%s', demo = '%s', release_directory = '%s', issues = %d, components = '%s', mail = '%s', mail_digest = '%s', mail_reminder = %d, mail_copy = '%s', mail_copy_filter = '%s', mail_copy_filter_state = '%s', version = %d, help = '%s', screenshots = '%s', documentation = '%s', license = '%s' WHERE nid = %d", $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->mail_reminder, $node->mail_copy, $node->mail_copy_filter, $node->mail_copy_filter_state, $node->version, $node->help, $node->screenshots, $node->documentation, $node->license, $node->nid);
  project_release_scan_directory($node->uri);
}

function project_project_delete($node) {
  db_query('DELETE FROM {project_projects} WHERE nid = %d', $node->nid);
}

function project_project_access($op, $node) {
  global $user;

  switch ($op) {
    case 'view':
      return user_access('access projects');
    case 'create':
      return user_access('maintain projects') && $user->uid;
    case 'update':
    case 'delete':
      return $user->uid && ($user->uid == $node->uid || user_access('administer projects'));
  }
}

function project_project_retrieve($key = 0) {
  if ($key) {
    if (is_numeric($key)) {
      $node = node_load(array('nid' => $key, 'type' => 'project_project'));
    }
    else {
      $node = node_load(array('nid' => db_result(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s'", $key), 0), 'type' => 'project_project'));
    }
  }
  return $node;
}

function project_developers($nid = 0) {
  if ($project = node_load(array('nid' => $nid))) {
    $output = module_invoke('cvs', 'get_project_contributors', $nid);
    drupal_set_title(t('Developers for %name', array('%name' => $project->title)));
    print theme('page', $output);
  }
  else {
    drupal_not_found();
  }
}

function project_cvs($nid = 0) {
  if ($project = node_load(array('nid' => $nid))) {
    $_REQUEST['nid'] = $nid;
    $output = module_invoke('cvs', 'show_messages');
    drupal_set_title(t('CVS messages for %name', array('%name' => $project->title)));
    print theme('page', $output);
  }
  else {
    drupal_not_found();
  }
}

?>
