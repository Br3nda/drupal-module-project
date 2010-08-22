<?php

/**
 * Provide a default argument of the currently relevant project type.
 */
class project_plugin_argument_default_project_type extends views_plugin_argument_default {
  function get_argument() {
    // If we're on one of the project menu items (e.g. for project browsing),
    // the 2nd element in the URL *should* be the project type.
    if (arg(0) == 'project') {
      return arg(1);
    }

    // If we're viewing a node, see if it's a project node, and if so, figure
    // out its project type.
    if (arg(0) == 'node') {
      $node = node_load(arg(1));
      if ($node->type == 'project_project') {
        // Grab only the top-level terms.
        $tree = taxonomy_get_tree(_project_get_vid(), 0, -1, 1);
        foreach ($tree as $project_type_term) {
          if (isset($node->taxonomy[$project_type_term->tid])) {
            return $project_type_term->name;
          }
        }
      }
    }
  }
}
