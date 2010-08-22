<?php
// $Id: project_release.views_default.inc,v 1.7 2010/08/22 00:14:51 dww Exp $

/**
 * @file
 * Scan the 'default_views' subdirectory for default views by project_release.
 */

/**
 * Implement hook_views_default_views().
 */
function project_release_views_default_views() {
  // Search the "default_views" subdirectory for files ending in .view.php.
  $files = file_scan_directory(drupal_get_path('module', 'project_release'). '/views/default_views', 'view\.php$');
  foreach ($files as $absolute => $file) {
    // This is not require_once because it's possible that
    // hook_views_default_views() gets called more than once.
    require $absolute;
    if (isset($view)) {
      // $file->name has the ".php" stripped off, but still has the ".view".
      $view_name = substr($file->name, 0, strrpos($file->name, '.'));
      $views[$view_name] = $view;
    }
  }
  return $views;
}

/**
 * Implement hook_views_default_views_alter().
 *
 * This injects project_release-specific features into views provided
 * by the project.module.
 */
function _project_release_views_default_views_alter(&$views) {
  if (isset($views['project_index'])) {
    $views['project_index']->set_display('default');
    $views['project_index']->display_handler->set_option('relationships', array(
      'supported_releases_rel' => array(
        'label' => 'supported versions',
        'required' => 0,
        'id' => 'supported_releases_rel',
        'table' => 'project_projects',
        'field' => 'supported_releases_rel',
        'relationship' => 'none',
      ),
    ));
    $filters = $views['project_index']->display_handler->get_option('filters');
    $filters['tid'] = array(
      'operator' => 'in',
      'value' => array(),
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'tid_op',
        'identifier' => 'api_compatibility',
        'label' => 'Filter by API compatibility',
        'optional' => 1,
        'single' => 1,
        'remember' => 0,
        'reduce' => 0,
      ),
      'type' => 'select',
      'id' => 'tid',
      'table' => 'project_release_supported_versions',
      'field' => 'tid',
      'relationship' => 'supported_releases_rel',
    );
    $views['project_index']->display_handler->set_option('filters', $filters);
    $views['project_index']->destroy();
  }
}
