<?php

/**
 * @file
 * drush_make extension for integrating with the drupal.org packaging system.
 *
 * This extension serves the following purposes:
 *  - Adds the --drupal-org and --build-run-key command line options,
 *    which activate the functionality in the rest of the code.
 *  - Validates .make files to ensure they comply with drupal.org packaging
 *    requirements.
 *  - Restricts download locations to those approved for drupal.org packages.
 *  - Implements a custom logging function for error reporting to the packaging
 *    script.
 *  - Builds a list of package contents (nids of package items), and stores
 *    for the packaging system to use.
 */

// URI of the 'drush_make on drupal.org' handbook page.
// TODO: fill in this constant with the nid when it's written.
define('DRUSH_MAKE_DO_DOCUMENTATION_LINK', 'http://drupal.org/node/642116');

/**
 * Implement COMMAND_drush_make_init().
 */
function drush_make_d_o_drush_init($makefile, $base_path) {
  if (drush_get_option('drupal-org')) {
    // Remember the build path for this run.
    drush_set_context('DRUSH_MAKE_DO_PACKAGE_BUILD_PATH', $base_path);
    // Set up a custom error logger.
    drush_set_context('DRUSH_LOG_CALLBACK', 'drush_make_d_o_log_errors_to_file');
  }
  // If a build run key was passed, save it.
  if ($build_run_key = drush_get_option('build-run-key')) {
    drush_set_context('DRUSH_MAKE_DO_BUILD_RUN_KEY', $build_run_key);
  }
}

/**
 * Implement COMMAND_drush_make_command().
 */
function drush_make_d_o_drush_make_command(&$function, &$parameters, &$dependencies, $id, &$queue) {
  if (drush_get_option('drupal-org')) {
    switch ($function) {
      // No libraries on d.o.
      case 'drush_make_add_libraries':
        return FALSE;
      // Custom drupal.org validation for .make files.
      case 'drush_make_validate_info_file':
        $function = 'drush_make_d_o_validate_info_file';
        break;
      // Custom drupal.org processing for loading update XML files.
      case 'drush_make_updatexml':
        $function = 'drush_make_d_o_updatexml';
    }
  }
}

/**
 * Validates .make files to ensure they adhere to the restricted standards for
 * drupal.org.
 */
function drush_make_d_o_validate_info_file($info) {

  // The list of currently allowed top-leval attributes.
  $top_level_attribute_whitelist = array('core', 'core_release', 'projects');

  // The list of currently allowed project-level attributes.
  $project_attribute_whitelist = array('version', 'subdir');

  // This function is called to validate all .make files.  Since we don't want
  // to validate the .make file we auto-generate for stage one, check for the
  // special 'build_run_key' attribute, and skip if no validation is
  // required.
  if (drush_make_d_o_check_build_run_key($info)) {
    return $info;
  }

  // Assume no errors to start.
  $errors = FALSE;

  // Check for disallowed top-level attributes.
  foreach ($info as $attribute => $attribute_data) {
    if (!in_array($attribute, $top_level_attribute_whitelist)) {
      drush_make_error('BUILD_ERROR', dt("The top-level attribute '%attribute' is not allowed in drupal.org .make files", array('%attribute' => $attribute)));
      $errors = TRUE;
    }
  }

  // Missing core definition.
  if (empty($info['core'])) {
    drush_make_error('BUILD_ERROR', dt('No Drupal core version specified.'));
    $errors = TRUE;
  }
  // Check for bad core value.
  // It seems too brittle to make this check always current with the current
  // supported versions of core, so just do a basic format check.
  elseif (!preg_match('/^\d+\.x$/', $info['core'])) {
    drush_make_error('BUILD_ERROR', dt('Bad Drupal core version specified.'));
    $errors = TRUE;
  }
  // Missing core_release definition.
  if (empty($info['core_release'])) {
    drush_make_error('BUILD_ERROR', dt('No Drupal core_release attribute specified.'));
    $errors = TRUE;
  }
  // Check for bad core release value.
  // It seems too brittle to make this check always current with the current
  // supported versions of core, so just do a basic format check.
  elseif (drush_make_d_o_check_bad_tag(dt('Drupal core'), $info['core_release'])) {
    $errors = TRUE;
  }
  if (!empty($info['projects'])) {
    // Bad projects definition.
    if (!is_array($info['projects'])) {
      drush_make_error('BUILD_ERROR', dt("'projects' attribute incorrectly specified."));
      $errors = TRUE;
    }
    else {
      foreach ($info['projects'] as $project => $project_data) {
        // No specifying just project name.
        if (is_numeric($project)) {
          drush_make_error('BUILD_ERROR', dt('Automatic best release support for project %project is not supported -- you must specifiy a version.', array('%project' => $project_data)));
          $errors = TRUE;
        }
        else {
          // Convert the shorthand for version declarations.
          if (!is_array($project_data)) {
            $project_data = array('version' => $project_data);
          }
          else {
            // Version must be set.
            if (!isset($project_data['version'])) {
              drush_make_error('BUILD_ERROR', dt('No version specified for project %project -- version is required.', array('%project' => $project)));
              $errors = TRUE;
            }
            // Check for disallowed project-level attributes.
            foreach ($project_data as $attribute => $attribute_data) {
              if (!in_array($attribute, $project_attribute_whitelist)) {
                drush_make_error('BUILD_ERROR', dt("The project-level attribute '%attribute' is not allowed in drupal.org .make files", array('%attribute' => $attribute)));
                $errors = TRUE;
              }
            }
          }
          // Check for bad project tag.
          if (drush_make_d_o_check_bad_tag($project, $project_data['version'])) {
            $errors = TRUE;
          }
        }
      }
    }
  }
  // Abort if any errors were found.
  if ($errors) {
    drush_make_error('BUILD_ERROR', dt('A .make file in your profile failed the <a href="@doc_link">drupal.org validation checks</a>.', array('@doc_link' => DRUSH_MAKE_DO_DOCUMENTATION_LINK)));
    return FALSE;
  }
  else {
    return $info;
  }
}

/**
 * Checks for bad release tags.
 *
 * @param $name
 *   The display name of the project.
 * @param $tag
 *   The tag to test.
 * @return
 * TRUE if the tag is bad, FALSE otherwise.
 */
function drush_make_d_o_check_bad_tag($name, $tag) {;
  // Development snapshots not allowed.
  if (preg_match('/^\d+\..*-dev/', $tag)) {
    drush_make_error('BUILD_ERROR', dt('%project branch releases not allowed.', array('%project' => $name)));
    return TRUE;
  }
  // Basic sanity checking on release tags. At this point we can't easily know
  // if the release is valid, but it's better than nothing.
  elseif (!preg_match('/^\d+\.\d+(-[a-z0-9]+)?$/', $tag)) {
    drush_make_error('BUILD_ERROR', dt('%project release format incorrect.', array('%project' => $name)));
    return TRUE;
  }
  return FALSE;
}

/**
 * Custom implementation of drush_make_updatexml() -- only allows loading
 * update XML files from the default location, which for our purposes is
 * updates.drupal.org.
 */
function drush_make_d_o_updatexml($project, $queue) {
  // Only stage one .make files should be able to bypass the update XML code
  // below, so check for the build_run_key here as well.
  if (drush_make_d_o_check_build_run_key($project) && isset($project['download']) && isset($project['type'])) {
    return $project;
  }
  // First, get the release history.
  @$release_history = simplexml_load_file(DRUSH_MAKE_UPDATE_DEFAULT_URL . '/' . $project['name'] . '/' . $project['core']);
  print_r($release_history);
  if (!$release_history) {
    drush_make_error('XML_ERROR', dt("Could not retrieve version information for %project.", array('%project' => $project['name'])));
    return FALSE;
  }
  drush_log(dt('Project information for %project retrieved.', array('%project' => $project['name'])), 'ok');
  $project['release_history'] = $release_history;
  if (!isset($project['type'])) {
    // Determine the project type.
    $term_map = array('Modules' => 'module', 'Themes' => 'theme', 'Drupal project' => 'core', 'Installation profiles' => 'profile');
    // Iterate through all terms related to this project.
    foreach ($release_history->terms->term as $term) {
      // If the term is eather 'Modules' or 'Themes', add it.
      if (in_array((string) $term->value, array_keys($term_map))) {
        $project['type'] = $term_map[(string)$term->value];
        break;
      }
    }
    if (!isset($project['type'])) {
      drush_make_error('BUILD_ERROR', dt("Unable to determine project type for %project.", array('%project' => $project['name'])));
      return FALSE;
    }
  }

  // Process the file download data, and record the item_nid for the package.
  if ($project = drush_make_update_xml_download($project)) {
    if ($project = drush_make_d_o_write_package_item_nid($project)) {
      return $project;
    }
  }
  return FALSE;
}

/**
 * Finds the item nid of the release that's being included in the package, and
 * stores it in the package contents file.
 *
 * Release/version logic ripped off from drush_make_update_xml_download().
 */
function drush_make_d_o_write_package_item_nid($project) {
  // Make an array of releases.
  foreach ($project['release_history']->releases->release as $release) {
    $version = (string) $release->version_major;
    if ((string) $release->version_patch) {
      $version .= '.' . (string) $release->version_patch;
    }
    else {
      $version .= '.0';
    }
    if ($extra_version = (string) $release->version_extra) {
      $version .= '-' . $extra_version;
    }
    $releases[$version] = basename($release->release_link);
  }

  // Write the item's nid to the package contents file.
  if ($project['version'] && ($item_nid = $releases[$project['version']])) {
    $package_build_path = dirname(drush_get_context('DRUSH_MAKE_DO_PACKAGE_BUILD_PATH', NULL));
    if (file_put_contents("$package_build_path/package_contents.txt", "$item_nid\n", FILE_APPEND)) {
      return $project;
    }
  }
  return FALSE;
}

/**
 * Custom logging function for packaging errors. Logs all error messages to a
 * build_errors.txt file in the root of the package build.  See drush_log()
 * for a description of the $entry array.
 */
function drush_make_d_o_log_errors_to_file($entry) {
  $package_build_path = dirname(drush_get_context('DRUSH_MAKE_DO_PACKAGE_BUILD_PATH', NULL));
  if ($entry['type'] == 'error' || $entry['type'] == 'failed') {
    file_put_contents("$package_build_path/build_errors.txt", $entry['message'] . "\n", FILE_APPEND);
  }
}

/**
 * Inspects an attribute array for a matching build run key.
 *
 * @param $attribute_data
 *   The array of attribute data to inspect.
 * @return
 *   TRUE if a build run key attribute exists, and it matches the build run
 *   key passed into drush, FALSE otherwise.
 */
function drush_make_d_o_check_build_run_key($attribute_data) {
  if (isset($attribute_data['build_run_key']) && $attribute_data['build_run_key'] == drush_get_context('DRUSH_MAKE_DO_BUILD_RUN_KEY', NULL)) {
    return TRUE;
  }
  return FALSE;
}

