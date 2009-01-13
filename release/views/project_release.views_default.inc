<?php
// $Id: project_release.views_default.inc,v 1.1 2009/01/13 17:47:09 dww Exp $

/**
 * @file
 * Contains default views on behalf of the project_release module.
 */

/**
 * Implementation of hook_default_view_views().
 */
function project_release_views_default_views() {
  $view = new view;
  $view->name = 'project_release_by_project';
  $view->description = 'List the project release nodes associated with a particular project.';
  $view->tag = 'project_release';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = FALSE;
  $view->api_version = 2;
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('fields', array(
    'nid' => array(
      'id' => 'nid',
      'table' => 'node',
      'field' => 'nid',
      'label' => 'Nid',
      'relationship' => 'none',
      'link_to_node' => 0,
    ),
    'title' => array(
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'label' => 'Title',
      'relationship' => 'none',
      'link_to_node' => 0,
    ),
    'version' => array(
      'id' => 'version',
      'table' => 'project_release_nodes',
      'field' => 'version',
      'label' => 'Version string',
      'relationship' => 'none',
      'link_to_node' => 1,
    ),
  ));
  $handler->override_option('sorts', array(
    'created' => array(
      'id' => 'created',
      'table' => 'node',
      'field' => 'created',
      'order' => 'DESC',
      'granularity' => 'day',
      'relationship' => 'none',
    ),
    'name' => array(
      'id' => 'name',
      'table' => 'term_data',
      'field' => 'name',
      'order' => 'DESC',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('arguments', array(
    'pid' => array(
      'default_action' => 'not found',
      'style_plugin' => 'default_summary',
      'style_options' => array(
        'count' => TRUE,
        'override' => FALSE,
        'items_per_page' => 25,
      ),
      'wildcard' => 'all',
      'wildcard_substitution' => 'All',
      'title' => 'Releases for %1',
      'default_argument_type' => 'fixed',
      'default_argument' => '',
      'validate_type' => 'node',
      'validate_fail' => 'empty',
      'break_phrase' => 0,
      'not' => 0,
      'id' => 'pid',
      'table' => 'project_release_nodes',
      'field' => 'pid',
      'relationship' => 'none',
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
        'project_project' => 'project_project',
        'project_release' => 0,
        'book' => 0,
        'page' => 0,
        'story' => 0,
      ),
      'validate_argument_vocabulary' => array(
        '3' => 0,
        '5' => 0,
        '2' => 0,
        '1' => 0,
        '4' => 0,
      ),
      'validate_argument_type' => 'tid',
      'validate_argument_php' => '',
      'default_argument_user' => 0,
      'validate_argument_node_access' => 1,
      'validate_argument_nid_type' => 'nid',
      'default_options_div_prefix' => '',
      'validate_argument_project_term_vocabulary' => array(
        '1' => 0,
      ),
      'validate_argument_project_term_argument_type' => 'tid',
      'validate_argument_project_term_argument_action_top_without' => 'pass',
      'validate_argument_project_term_argument_action_top_with' => 'pass',
      'validate_argument_project_term_argument_action_child' => 'pass',
    ),
  ));
  $handler->override_option('filters', array(
    'type' => array(
      'id' => 'type',
      'table' => 'node',
      'field' => 'type',
      'operator' => 'in',
      'value' => array(
        'project_release' => 'project_release',
      ),
      'group' => 0,
      'exposed' => FALSE,
      'expose' => array(
        'operator' => FALSE,
        'label' => '',
      ),
      'relationship' => 'none',
      'expose_button' => array(
        'button' => 'Expose',
      ),
    ),
    'tid' => array(
      'id' => 'tid',
      'table' => 'term_node',
      'field' => 'tid',
      'operator' => 'or',
      'value' => array(),
      'group' => 0,
      'exposed' => TRUE,
      'expose' => array(
        'operator' => '',
        'identifier' => 'api',
        'label' => 'API Version',
        'optional' => 1,
        'single' => 0,
        'remember' => 1,
      ),
      'type' => 'select',
      'vid' => '2',
      'relationship' => 'none',
      'expose_button' => array(
        'button' => 'Hide',
      ),
      'reduce_duplicates' => 0,
    ),
    'status' => array(
      'id' => 'status',
      'table' => 'node',
      'field' => 'status',
      'operator' => '=',
      'value' => 1,
      'group' => 0,
      'exposed' => FALSE,
      'expose' => array(
        'operator' => FALSE,
        'label' => '',
      ),
      'relationship' => 'none',
      'expose_button' => array(
        'button' => 'Expose',
      ),
    ),
  ));
  $handler->override_option('access', array(
    'type' => 'none',
    'role' => array(),
    'perm' => 'access projects',
  ));
  $handler->override_option('empty', 'There are no published releases for this project.');
  $handler->override_option('empty_format', '1');
  $handler->override_option('items_per_page', 20);
  $handler->override_option('use_pager', '1');
  $handler->override_option('row_plugin', 'node');
  $handler->override_option('row_options', array(
    'teaser' => 1,
    'links' => 1,
  ));
  $handler = $view->new_display('page', 'Releases for *project*', 'page');
  $handler->override_option('path', 'node/%/release');
  $handler->override_option('menu', array(
    'type' => 'none',
    'title' => 'Test title',
    'weight' => '0',
  ));
  $handler->override_option('tab_options', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $handler = $view->new_display('feed', 'Feed', 'feed');
  $handler->override_option('row_plugin', 'node_rss');
  $handler->override_option('row_options', array(
    'item_length' => 'default',
  ));
  $handler->override_option('path', 'node/%/release/feed');
  $handler->override_option('menu', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $handler->override_option('tab_options', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $handler->override_option('displays', array(
    'page' => 'page',
    'default' => 0,
  ));
  $views[$view->name] = $view;

  return $views;
}

