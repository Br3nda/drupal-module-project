<?php
// $Id: project_package.views_default.inc,v 1.2 2010/02/18 00:34:26 dww Exp $

/**
 * @file
 * Scan the 'default_views' subdirectory for default views by project_package.
 */

/**
 * Implement hook_views_default_views().
 */
function project_package_views_default_views() {
  // Search the "default_views" subdirectory for files ending in .view.php.
  $files = file_scan_directory(drupal_get_path('module', 'project_package'). '/views/default_views', 'view\.php$');
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

