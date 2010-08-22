<?php
// $Id: project_index.view.php,v 1.1 2010/08/22 00:14:51 dww Exp $

/**
 * @file
 * An index of all projects of a certain project type by title.
 */

$view = new view;
$view->name = 'project_index';
$view->description = 'Listing of all projects by title';
$view->tag = '';
$view->view_php = '';
$view->base_table = 'node';
$view->is_cacheable = FALSE;
$view->api_version = 2;
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('fields', array(
  'title' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'link_to_node' => 1,
    'exclude' => 0,
    'id' => 'title',
    'table' => 'node',
    'field' => 'title',
    'relationship' => 'none',
  ),
));
$handler->override_option('sorts', array(
  'title' => array(
    'order' => 'ASC',
    'id' => 'title',
    'table' => 'node',
    'field' => 'title',
    'relationship' => 'none',
  ),
));
$handler->override_option('arguments', array(
  'tid' => array(
    'default_action' => 'not found',
    'style_plugin' => 'default_summary',
    'style_options' => array(),
    'wildcard' => 'all',
    'wildcard_substitution' => 'All',
    'title' => '%1 index',
    'breadcrumb' => '',
    'default_argument_type' => 'fixed',
    'default_argument' => '',
    'validate_type' => 'project_type_term',
    'validate_fail' => 'not found',
    'break_phrase' => 0,
    'add_table' => 0,
    'require_value' => 0,
    'reduce_duplicates' => 0,
    'set_breadcrumb' => 0,
    'id' => 'tid',
    'table' => 'term_node',
    'field' => 'tid',
    'validate_user_argument_type' => 'uid',
    'validate_user_roles' => array(
    ),
    'relationship' => 'none',
    'default_options_div_prefix' => '',
    'default_argument_fixed' => '',
    'default_argument_user' => 0,
    'default_argument_php' => '',
    'validate_argument_node_type' => array(
    ),
    'validate_argument_node_access' => 0,
    'validate_argument_nid_type' => 'nid',
    'validate_argument_vocabulary' => array(
    ),
    'validate_argument_type' => 'convert',
    'validate_argument_transform' => 0,
    'validate_user_restrict_roles' => 0,
    'validate_argument_project_term_argument_type' => 'convert',
    'validate_argument_project_term_argument_action_top_without' => 'pass',
    'validate_argument_project_term_argument_action_top_with' => 'pass',
    'validate_argument_project_term_argument_action_child' => 'fail',
    'validate_argument_php' => '',
  ),
));
$handler->override_option('filters', array(
  'status' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status',
    'table' => 'node',
    'field' => 'status',
    'relationship' => 'none',
  ),
  'type' => array(
    'operator' => 'in',
    'value' => array(
      'project_project' => 'project_project',
    ),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'type',
    'table' => 'node',
    'field' => 'type',
    'relationship' => 'none',
  ),
));
$handler->override_option('access', array(
  'type' => 'none',
));
$handler->override_option('cache', array(
  'type' => 'none',
));
$handler->override_option('items_per_page', 0);
$handler->override_option('distinct', 1);
$handler->override_option('style_plugin', 'list');
$handler->override_option('style_options', array(
  'grouping' => '',
  'type' => 'ol',
));
$handler = $view->new_display('page', 'Page', 'page_1');
$handler->override_option('path', 'project/%/index');
$handler->override_option('menu', array(
  'type' => 'none',
  'title' => '',
  'description' => '',
  'weight' => 0,
  'name' => 'navigation',
));
$handler->override_option('tab_options', array(
  'type' => 'none',
  'title' => '',
  'description' => '',
  'weight' => 0,
  'name' => 'navigation',
));
$handler = $view->new_display('block', 'Block', 'block_1');
$handler->override_option('arguments', array(
  'tid' => array(
    'default_action' => 'default',
    'style_plugin' => 'default_summary',
    'style_options' => array(),
    'wildcard' => 'all',
    'wildcard_substitution' => 'All',
    'title' => '%1 index',
    'breadcrumb' => '',
    'default_argument_type' => 'project_plugin_argument_default_project_type',
    'default_argument' => '',
    'validate_type' => 'project_type_term',
    'validate_fail' => 'not found',
    'break_phrase' => 0,
    'add_table' => 0,
    'require_value' => 0,
    'reduce_duplicates' => 0,
    'set_breadcrumb' => 0,
    'id' => 'tid',
    'table' => 'term_node',
    'field' => 'tid',
    'validate_user_argument_type' => 'uid',
    'validate_user_roles' => array(
    ),
    'relationship' => 'none',
    'default_options_div_prefix' => '',
    'default_argument_fixed' => '',
    'default_argument_user' => 0,
    'default_argument_php' => '',
    'validate_argument_node_type' => array(
    ),
    'validate_argument_node_access' => 0,
    'validate_argument_nid_type' => 'nid',
    'validate_argument_vocabulary' => array(
    ),
    'validate_argument_type' => 'convert',
    'validate_argument_transform' => 0,
    'validate_user_restrict_roles' => 0,
    'validate_argument_project_term_argument_type' => 'convert',
    'validate_argument_project_term_argument_action_top_without' => 'pass',
    'validate_argument_project_term_argument_action_top_with' => 'pass',
    'validate_argument_project_term_argument_action_child' => 'fail',
    'validate_argument_php' => '',
    'override' => array(
      'button' => 'Use default',
    ),
  ),
));
$handler->override_option('items_per_page', 5);
$handler->override_option('use_more', 1);
$handler->override_option('use_more_always', 1);
$handler->override_option('use_more_text', 'View full index');
$handler->override_option('style_options', array(
  'grouping' => '',
  'type' => 'ul',
));
$handler->override_option('block_description', '');
$handler->override_option('block_caching', -1);
