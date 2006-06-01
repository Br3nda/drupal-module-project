<?php
/* $Id: project.inc,v 1.80 2006/06/01 08:28:03 dww Exp $ */

/**
 * Implementation of hook_form().
 */
function project_project_form(&$node) {
  global $user;

  if (is_array($node->components)) {
    $node->components = implode("\n", $node->components);
  }
  elseif (empty($node->components)) {
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

  $form['#prefix'] = '<div class="project">';

  /* Project taxonomy */
  if (project_use_taxonomy()) {
    $tree = taxonomy_get_tree(_project_get_vid());
    $top_level = array();
    $options = array();
    foreach ($tree as $i => $term) {
      if ($term->parents[0] == 0) {
        $last_top = $term->tid;
        $top_level[$term->tid] = $term->name;
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
    foreach ($options as $tid => $values) {
      $form['project_taxonomy'][$tid] = array(
        '#title' => t('%type categories', array('%type' => $top_level[$tid])),
        '#type' => 'select',
        '#multiple' => TRUE,
        '#options' => $values,
        '#default_value' => $current_options,
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

  /* Issue properties */
  $form['issue'] = array(
    '#type' => 'fieldset',
    '#title' => t('Issue information'),
    '#collapsible' => TRUE,
  );
  $form['issue']['issue'] = array(
    '#type' => 'item',
    '#title' => t('Issue tracker'),
  );
  $form['issue']['issues'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable issue tracker'),
    '#return_value' => 1,
    '#default_value' => isset($node->issues) ? $node->issues : 1,
    '#description' => t('Let users submit bug requests, patches, feature requests, support requests, etc.'),
  );
  if ($releases = project_release_load($node)) {
    $form['issue']['version'] = array(
      '#type' => 'select',
      '#title' => t('Default version'),
      '#default_value' => $node->version,
      '#options' => $releases,
      '#description' => t('Default version for downloading and for submitting issues.'),
    );
  }
  $form['issue']['components'] = array(
    '#type' => 'textarea',
    '#title' => t('Components'),
    '#default_value' => $node->components,
    '#cols' => 20,
    '#rows' => 5,
  );
  $form['issue']['help'] = array(
    '#type' => 'textarea',
    '#title' => t('Submission guidelines'),
    '#default_value' => $node->help,
    '#cols' => 20,
    '#rows' => 5,
  );

  /* E-mail options */
  $form['email'] = array(
    '#type' => 'fieldset',
    '#title' => t('E-mail options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['email']['mail_digest'] = array(
    '#type' => 'textfield',
    '#title' => t('Weekly critical issues report'),
    '#default_value' => $node->mail_digest,
    '#size' => 20,
    '#maxlength' => 255,
    '#description' => t('To get a weekly digest of critical issues specify an e-mail address.'),
  );
  $form['email']['mail_copy'] = array(
    '#type' => 'textfield',
    '#title' => t('Issues e-mail address'),
    '#default_value' => $node->mail_copy,
    '#size' => 20,
    '#maxlength' => 255,
    '#description' => t('If you wish to receive a copy of all the issues to a central location specify an address here.'),
  );
  if (user_access('administer projects')) {
    $options = project_issue_category();
    $form['email']['categories'] = array(
      '#type' => 'item',
      '#title' => t('Categories'),
      '#description' => t('Which issue categories to e-mail. If none is checked all categories will be posted.'),
    );
    $form['email']['mail_copy_filter']['#tree'] = TRUE;

    foreach ($options as $key => $choice) {
      $form['email']['mail_copy_filter'][$key] = array(
        '#type' => 'checkbox',
        '#title' => $choice,
        '#return_value' => $key,
        '#default_value' => $node->mail_copy_filter[$key],
      );
    }
    $options = project_issue_state();
    $form['email']['states'] = array(
      '#type' => 'item',
      '#title' => t('States'),
      '#description' => t('Which issue states to e-mail. If none is checked all states will be posted.'),
    );
    $form['email']['mail_copy_filter_state']['#tree'] = TRUE;
    foreach ($options as $key => $choice) {
      $form['email']['mail_copy_filter_state'][$key] = array(
        '#type' => 'checkbox',
        '#title' => $choice,
        '#return_value' => $key,
        '#default_value' => $node->mail_copy_filter_state[$key],
      );
    }

    $form['email']['reminder'] = array(
      '#type' => 'item',
      '#title' => t('Monthly reminder'),
    );
    $form['email']['mail_reminder'] = array(
      '#type' => 'checkbox',
      '#title' => t('Send monthly reminders to users'),
      '#return_value' => 1,
      '#default_value' => $node->mail_reminder,
      '#description' => t('Enabling this will send a monthly reminder to users that have open issues registered.'),
    );
  }

  $form['#suffix'] = '</div>';
  return $form;
}

/**
 * Implementation of form_alter. This removes the work of
 * taxonomy.module's form_alter() so we can do our own taxonomy
 * selection. 
 */
function project_form_alter($form_id, &$form) {
  if ($form_id == 'project_project_node_form') {
    $vid = _project_get_vid();
    if (isset($form['taxonomy'][$vid])) {
      unset($form['taxonomy'][$vid]);
    }
    // If there are no children elements, we should unset the entire
    // thing so we don't end up with an empty fieldset.
    if (!element_children($form['taxonomy'])) {
      unset($form['taxonomy']);
    }
  }
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
        if (!empty($node->$tid)) {
          form_set_error($tid, t('Project type %project_type was selected, you can not use values from %invalid_type categories', array('%project_type' => theme('placeholder', $top_level[$node->project_type]), '%invalid_type' => theme('placeholder', $top_level[$tid]))));
        }
      }
    }
  }
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
  $result = db_query(db_rewrite_sql('SELECT t.tid, t.* FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid INNER JOIN {term_node} r ON t.tid = r.tid WHERE h.parent = 0 AND t.vid = %d AND r.nid = %d', 't', 'tid'), _project_get_vid(), $node->nid);
  $term = db_fetch_object($result);
  $breadcrumb[] = array('path' => 'project/'. $term->name, 'title' => $term->name);

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
      // Determine most suitable release
      $overview = variable_get('project_release_overview', -1);
      foreach ($releases as $rid => $version) {
        // If the project has a preferred release, choose that.
        if ($rid == $node->version) {
          $release = $rid;
          break;
        }
        // If the version matches the default on the overview, prefer the first.
        if (!isset($release) && strpos($version, $overview) === 0) {
          $release = $rid;
        }
      }
      // Fallback to the latest
      if (!isset($release)) {
        reset($releases);
        list($release) = each($releases);
      }
      $release = project_release_load($release);

      // Download section
      $links = array();

      $links[] = '<a href="'. base_path() . $release->path .'">' . t('Download latest release (%version, %date, %size)', array('%version' => $release->version, '%date' => format_date($release->changed, 'small'), '%size' => format_size(filesize($release->path))))  . '</a>';

      if ($release->changes) {
        $links[] = l(t('Read release notes'), 'node/'. $node->nid .'/release', null, null, 'version-'. $release->version);
      }
      if (count($releases) > 1) {
        $links[] = l(t('View other releases'), 'node/'. $node->nid .'/release');
      }
    }
    if (node_access('update', $node) && !variable_get('project_release_directory', '')) {
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

    if (module_exist('forum') && ($support_forum = variable_get('project_support_forum', ''))) {
      $links[] = l(t('Support forum'), 'forum/' . $support_forum);
    }

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
      $links[] = l(t('View pending patches'), 'project/issues/'. $node->uri, null, 'states=8,13,14', null);
      $links[] = l(t('View available tasks'), 'project/issues/'. $node->uri, null, 'categories=task', null);
      $links[] = l(t('View all issues'), 'project/issues/'. $node->uri);
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

    $node->body .= $output;
  }
}

function project_project_releases() {
  $node = node_load(arg(1));

  // Breadcrumb navigation
  $breadcrumb[] = array('path' => 'node/'. $node->nid .'/', 'title' => $node->title);
  $breadcrumb[] = array('path' => 'node/'. arg(1) .'/release', 'title' => t('Releases'));
  $breadcrumb = project_project_set_location($node, $breadcrumb);

  menu_set_location($breadcrumb);

  $output = project_release_list($node);

  drupal_set_title(check_plain($node->title));
  return $output;
}

function project_project_load($node) {
  $project = db_fetch_object(db_query('SELECT * FROM {project_projects} WHERE nid = %d', $node->nid));
  $project->components = unserialize($project->components);
  $project->mail_copy_filter = unserialize($project->mail_copy_filter);
  $project->mail_copy_filter_state = unserialize($project->mail_copy_filter_state);

  return $project;
}

function project_nodeapi(&$node, $op, $arg) {
  switch ($op) {
    case 'insert':
      if ($node->type == 'project_project') {
        _project_save_taxonomy($node);
      }
      break;
    case 'update':
      if ($node->type == 'project_project') {
        _project_save_taxonomy($node);
      }
      break;
  }
}

function project_project_insert($node) {
  project_project_convert_components($node);
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);
  $node->mail_copy_filter_state = serialize($node->mail_copy_filter_state);

  db_query("INSERT INTO {project_projects} (nid, uri, homepage, changelog, cvs, demo, release_directory, issues, components, mail, mail_digest, mail_reminder, mail_copy, mail_copy_filter, mail_copy_filter_state, version, help, screenshots, documentation, license) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s')", $node->nid, $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->reminder, $node->mail_copy, $node->mail_copy_filter, $node->mail_copy_filter_state, $node->version, $node->help, $node->screenshots, $node->documentation, $node->license);
  project_release_scan_directory($node->uri);
}

function project_project_update($node) {
  project_project_convert_components($node);
  $node->components = serialize($node->components);
  $node->mail_copy_filter = serialize($node->mail_copy_filter);
  $node->mail_copy_filter_state = serialize($node->mail_copy_filter_state);

  db_query("UPDATE {project_projects} SET uri = '%s', homepage = '%s', changelog = '%s', cvs = '%s', demo = '%s', release_directory = '%s', issues = %d, components = '%s', mail = '%s', mail_digest = '%s', mail_reminder = %d, mail_copy = '%s', mail_copy_filter = '%s', mail_copy_filter_state = '%s', version = %d, help = '%s', screenshots = '%s', documentation = '%s', license = '%s' WHERE nid = %d", $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->mail_reminder, $node->mail_copy, $node->mail_copy_filter, $node->mail_copy_filter_state, $node->version, $node->help, $node->screenshots, $node->documentation, $node->license, $node->nid);
  project_release_scan_directory($node->uri);
}

function project_project_delete($node) {
  db_query('DELETE FROM {project_projects} WHERE nid = %d', $node->nid);
  db_query('DELETE FROM {project_issues} WHERE pid = %d', $node->nid);
}

function project_project_access($op, $node) {
  global $user;

  switch ($op) {
    case 'view':
      return user_access('access projects') || (user_access('access own projects') && $node->uid == $user->uid);
    case 'create':
      if ($user->uid && user_access('maintain projects')) {
        if (module_exist('cvs') && variable_get('cvs_restrict_project_creation',0)) {
          return db_result(db_query("SELECT uid FROM {cvs_accounts} WHERE uid = %d", $user->uid), 0);
        }
        else {
          return TRUE;
        }
      }
      return FALSE;
    case 'update':
    case 'delete':
      return $user->uid && ($user->uid == $node->uid || user_access('administer projects'));
  }
}

function project_project_convert_components(&$node) {
  // Transform components into array.
  if (!is_array($node->components)) {
    if ($node->components) {
      $node->components = explode("\n", str_replace("\r", '', $node->components));
    }
    else {
      $node->components = array();
    }
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
    $output = module_invoke('cvs', 'get_project_contributors', $nid);
    drupal_set_title(t('Developers for %name', array('%name' => $project->title)));
    return $output;
  }
  else {
    drupal_not_found();
  }
}

function project_cvs($nid = 0) {
  if ($project = node_load($nid)) {
    $_REQUEST['nid'] = $nid;
    $output = module_invoke('cvs', 'show_messages');
    drupal_set_title(t('CVS messages for %name', array('%name' => $project->title)));
    return $output;
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
    if (isset($node->$tid)) {
      foreach ($node->$tid as $tid) {
        _project_db_save_taxonomy($node->nid, $tid);
      }
    }
  }
}

function _project_db_save_taxonomy($nid, $tid) {
  db_query('INSERT INTO {term_node} (nid, tid) VALUES (%d, %d)', $nid, $tid);
}
