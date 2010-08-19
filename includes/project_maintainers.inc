<?php
// $Id: project_maintainers.inc,v 1.1 2010/08/19 16:31:17 dww Exp $

/**
 * @file
 * Code for the node/N/maintainers tab on project nodes.
 */

/**
 * Build the form for the node/N/maintainers tab on project nodes.
 *
 * This form uses project_permission_load() which in turn invokes
 * hook_project_permission_info() to gather information about all the
 * per-project permissions defined by any enabled modules on the site. It
 * loops over all the current maintainers of the project and provides a
 * checkbox for each user/permission pair. As a special-case, the owner of the
 * project is automatically granted all permissions, so the row for them
 * includes all the checkboxes pre-selected and disabled.  Finally, there's an
 * auto-complete username text box and a set of checkboxes to allow the user
 * of this form to add a new maintainer and select their permissions.
 *
 * @param $form_state
 *   The Form API state of the form (set by drupal_get_form()).
 * @param $project
 *   The fully-loaded node object for the project to build the maintainers
 *   form for.
 *
 * @return
 *   A form definition array for use by the Form API.
 *
 * @see theme_project_maintainers_form()
 * @see project_maintainers_form_validate()
 * @see project_maintainers_form_submit()
 * @see project_permission_load()
 * @see hook_project_permission_info()
 * @see drupal_get_form()
 */
function project_maintainers_form($form_state, $project) {
  // Load all the info about per-project permissions on this site.
  $project_perms = project_permission_load();

  $form = array();
  $form['#tree'] = TRUE;
  $form['#header'] = array();
  $form['#project'] = $project;
  $form['#header']['username'] = array('data' => t('User'));
  foreach ($project_perms as $perm_name => $perm_info) {
    $form['#header'][$perm_name] = array('data' => $perm_info['title']);
  }
  $form['#header']['operations'] = array('data' => t('Operations'));

  if (!empty($project->project['maintainers'])) {
    foreach ($project->project['maintainers'] as $uid => $maintainer) {
      $form['maintainers'][$uid] = array();
      $form['maintainers'][$uid]['name'] = array(
        '#type' => 'value',
        '#value' => $maintainer['name'],
      );
      foreach ($project_perms as $perm_name => $perm_info) {
        $form['maintainers'][$uid]['permissions'][$perm_name] = array(
          '#type' => 'checkbox',
          '#default_value' => !empty($maintainer['permissions'][$perm_name]),
        );
      }
      $form['maintainers'][$uid]['operations'] = array();
      if ($uid == $project->uid) {
        // We special-case the project owner with disabled checkboxes.
        foreach ($project_perms as $perm_name => $perm_info) {
          $form['maintainers'][$uid]['permissions'][$perm_name]['#disabled'] = TRUE;
        }
        $form['maintainers'][$uid]['operations']['delete'] = array(
          '#value' => t('locked'),
        );
      }
      else {
        $form['maintainers'][$uid]['operations']['delete'] = array(
          '#value' => l(t('delete'), "node/$project->nid/maintainers/delete/$uid"),
        );
      }
    }
  }

  $form['new_maintainer'] = array();
  $form['new_maintainer']['user'] = array(
    '#type' => 'textfield',
    '#size' => 20,
    '#maxlength' => 40,
    '#autocomplete_path' => 'user/autocomplete',
  );
  // we'll fill this in with a real value during validate()
  $form['new_maintainer']['uid'] = array(
    '#type' => 'value',
    '#value' => 0,
  );
  foreach ($project_perms as $perm_name => $perm_info) {
    $form['new_maintainer']['permissions'][$perm_name] = array(
      '#type' => 'checkbox',
    );
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Update'));

  return $form;
}

/**
 * Render the final markup for the project maintainers form.
 *
 * @param $form
 *   The fully-built form array for the project maintainers form.
 *
 * @return
 *   String containing the markup to output for the maintainers form.
 *
 * @see theme()
 * @see project_maintainers_form()
 */
function theme_project_maintainers_form($form) {
  $output = '';

  $header = $form['#header'];
  $rows = array();

  // Render all the existing maintainers.
  if (is_array($form['maintainers'])) {
    foreach (element_children($form['maintainers']) as $uid) {
      $row = array();
      $account = new stdClass;
      $account->uid = $uid;
      $account->name = $form['maintainers'][$uid]['name']['#value'];
      $row[] = theme('username', $account);
      foreach (element_children($form['maintainers'][$uid]['permissions']) as $perm) {
        $row[] = drupal_render($form['maintainers'][$uid]['permissions'][$perm]);
      }
      $row[] = drupal_render($form['maintainers'][$uid]['operations']);
      if ($form['#project']->uid == $uid) {
        $owner_row = $row;
      }
      else {
        $rows[] = $row;
      }
    }
  }

  // Create the final row for adding a new maintainer.
  $row = array();
  $row[] = drupal_render($form['new_maintainer']['user']);
  foreach (element_children($form['new_maintainer']['permissions']) as $perm) {
    $row[] = drupal_render($form['new_maintainer']['permissions'][$perm]);
  }
  $row[] = ''; // Empty cell for the 'Operations' column on new maintainers.
  $rows[] = $row;

  // Always put the owner row at the top of the table.
  $rows = array_merge(array($owner_row), $rows);

  // Although using named keys in the $header array makes this form easier to
  // alter, theme_table() freaks out if the $header array has non-numeric
  // keys. So we ditch the keys at this point to avoid notices.
  $output .= theme('table', array_values($header), $rows);

  $project_perms = project_permission_load();
  $output .= '<dl class="description">';
  foreach ($project_perms as $perm => $perm_info) {
    $output .= ' <dt>' . $perm_info['title'] . '</dt>';
    $output .= '  <dd>' . $perm_info['description'] . '</dd>';
  }
  $output .= "</dl>\n";

  $output .= drupal_render($form);
  return $output;
}

/**
 * Validation callback for the project maintainers form.
 */
function project_maintainers_form_validate($form, &$form_state) {
  $new_maintainer = $form_state['values']['new_maintainer'];
  if (!empty($new_maintainer['user'])) {
    $user_result = db_fetch_object(db_query("SELECT name, uid FROM {users} WHERE name = '%s'", $new_maintainer['user']));
    if (empty($user_result->uid)) {
      form_set_error('new_maintainer][user', t('%user is not a valid user on this site.', array('%user' => $new_maintainer['user'])), 'error');
      return;
    }
    if (!empty($form['#project']->project['maintainers'][$user_result->uid])) {
      form_set_error('new_maintainer][user', t('%user is already a maintainer of this project.', array('%user' => $new_maintainer['user'])), 'error');
      return;
    }
    // Save the uid in the form so we don't have to look it up again at submit.
    form_set_value($form['new_maintainer']['uid'], $user_result->uid, $form_state);
  }
  else {
    foreach ($new_maintainer['permissions'] as $name => $value) {
      if (!empty($value)) {
        form_set_error('new_maintainer][user', t('You must specify a valid user name to grant permissions.'));
      }
    }
  }
}

/**
 * Submit callback for the project maintainers form.
 */
function project_maintainers_form_submit($form, &$form_state) {
  $project_nid = $form['#project']->nid;

  // Loop over all the maintainers and update their permissions accordingly.
  if (!empty($form_state['values']['maintainers'])) {
    foreach ($form_state['values']['maintainers'] as $uid => $maintainer) {
      // Just to be extra safe, always give the project owner full permissions.
      if ($uid == $form['#project']->uid) {
        $perms = array_fill_keys(array_keys(project_permission_load()), 1);
      }
      else {
        $perms = $maintainer['permissions'];
      }
      project_maintainer_save($project_nid, $uid, $perms);
    }
  }

  // See if we need to insert a record for a new maintainer.
  if (!empty($form_state['values']['new_maintainer']['uid'])) {
    project_maintainer_save($project_nid, $form_state['values']['new_maintainer']['uid'], $form_state['values']['new_maintainer']['permissions']);
  }

}

/**
 * Confirm form for removing a uid as a cvs maintainer from a given project.
 */
function project_maintainer_delete_confirm($form_state, $project, $user) {
  if ($user->uid == $project->uid) {
    drupal_set_message(t('You can not delete the project owner (!user) as a maintainer.', array('!user' => theme('username', $user))), 'error');
    return drupal_goto("node/$project->nid/maintainers/");
  }

  $form['nid'] = array('#type' => 'value', '#value' => $project->nid);
  $form['uid'] = array('#type' => 'value', '#value' => $user->uid);

  return confirm_form($form,
           t('Are you sure you want to delete !user as a maintainer of !project?',
             array(
               '!user' => theme('username', $user),
               '!project' => l($project->title, "node/$project->nid"),
             )),
           "node/$project->nid/maintainers",
           t('This action cannot be undone.'),
           t('Delete'),
           t('Cancel'));
}

/**
 * Delete the requested user as a maintainer.
 *
 * Invoked when the delete button on the confirm_form() page is pressed.
 */
function project_maintainer_delete_confirm_submit($form, &$form_state) {
  $nid = $form_state['values']['nid'];
  $uid = $form_state['values']['uid'];
  $user = user_load(array('uid' => $uid));

  project_maintainer_remove($nid, $uid);

  drupal_set_message(t('Removed !user as a maintainer.', array('!user' => theme('username', $user))));
  $form_state['redirect'] = "node/$nid/maintainers";
}
