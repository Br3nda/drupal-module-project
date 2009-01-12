<?php
// $Id: project.views_default.inc,v 1.1 2009/01/12 20:34:06 dww Exp $

/**
 * @file
 * Contains default views on behalf of the project module.
 */

/**
 * Implementation of hook_default_view_views().
 */
function project_views_default_views() {
  $view = new view;
  $view->name = 'project_types';
  $view->description = 'A list of project types with descriptions.';
  $view->tag = 'project';
  $view->view_php = '';
  $view->base_table = 'term_data';
  $view->is_cacheable = FALSE;
  $view->api_version = 2;
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('fields', array(
    'name' => array(
      'id' => 'name',
      'table' => 'term_data',
      'field' => 'name',
      'label' => '',
      'relationship' => 'none',
      'link_to_taxonomy' => 1,
    ),
    'description' => array(
      'id' => 'description',
      'table' => 'term_data',
      'field' => 'description',
      'label' => '',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('filters', array(
    'project_type' => array(
      'operator' => 'in',
      'value' => array(
        '1' => 1,
      ),
      'group' => 0,
      'exposed' => FALSE,
      'expose' => array(
        'operator' => FALSE,
        'label' => '',
      ),
      'include' => array(
        'top_level' => 'top_level',
      ),
      'id' => 'project_type',
      'table' => 'term_data',
      'field' => 'project_type',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('access', array(
    'type' => 'perm',
    'role' => array(),
    'perm' => 'browse project listings',
  ));
  $handler->override_option('title', 'Project types');
  $handler->override_option('empty_format', '1');
  $handler->override_option('items_per_page', 30);
  $handler->override_option('use_pager', '1');
  $handler->override_option('distinct', 1);
  $handler->override_option('style_plugin', 'list');
  $handler->override_option('style_options', array(
    'type' => 'ul',
  ));
  $handler->override_option('row_options', array(
    'inline' => array(),
    'separator' => '',
  ));
  $handler = $view->new_display('page', 'Page: Project types', 'page');
  $handler->override_option('path', 'project');
  $handler->override_option('menu', array(
    'type' => 'normal',
    'title' => 'Projects',
    'weight' => '0',
  ));
  $handler->override_option('tab_options', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $views[$view->name] = $view;

  $view = new view;
  $view->name = 'project_overview_2';
  $view->description = 'A view for browsing projects.';
  $view->tag = 'project';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = FALSE;
  $view->api_version = 2;
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('relationships', array(
    'project_release_version' => array(
      'label' => 'release',
      'required' => 0,
      'id' => 'project_release_version',
      'table' => 'node',
      'field' => 'project_release_version',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('fields', array(
    'title' => array(
      'label' => '',
      'link_to_node' => 1,
      'exclude' => 0,
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'relationship' => 'none',
    ),
    'changed' => array(
      'label' => 'Last changed',
      'date_format' => 'time ago',
      'custom_date_format' => '2',
      'exclude' => 0,
      'id' => 'changed',
      'table' => 'node',
      'field' => 'changed',
      'relationship' => 'none',
    ),
    'teaser' => array(
      'label' => '',
      'exclude' => 0,
      'id' => 'teaser',
      'table' => 'node_revisions',
      'field' => 'teaser',
      'relationship' => 'none',
    ),
    'download_table' => array(
      'label' => '',
      'exclude' => 0,
      'id' => 'download_table',
      'table' => 'node',
      'field' => 'download_table',
      'release_type' => 'official',
      'table_type' => 'supported',
      'first_column_title' => 'Version',
      'relationship' => 'none',
    ),
    'project_type_tid' => array(
      'label' => '',
      'type' => 'separator',
      'separator' => '  ',
      'empty' => '',
      'link_to_taxonomy' => 1,
      'limit' => 0,
      'vids' => array(
        '1' => 1,
      ),
      'exclude_top_level_terms' => 1,
      'exclude' => 0,
      'id' => 'project_type_tid',
      'table' => 'term_node',
      'field' => 'project_type_tid',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('sorts', array(
    'sticky' => array(
      'order' => 'DESC',
      'id' => 'sticky',
      'table' => 'node',
      'field' => 'sticky',
      'relationship' => 'none',
    ),
    'title' => array(
      'order' => 'ASC',
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'relationship' => 'none',
    ),
    'changed' => array(
      'order' => 'DESC',
      'granularity' => 'day',
      'id' => 'changed',
      'table' => 'node',
      'field' => 'changed',
      'relationship' => 'project_release_version',
    ),
    'title_1' => array(
      'order' => 'ASC',
      'id' => 'title_1',
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
      'title' => '%1',
      'default_argument_type' => 'fixed',
      'default_argument' => '',
      'validate_type' => 'project_type_term',
      'validate_fail' => 'not found',
      'break_phrase' => 0,
      'add_table' => 0,
      'require_value' => 0,
      'reduce_duplicates' => 1,
      'set_breadcrumb' => 0,
      'id' => 'tid',
      'table' => 'term_node',
      'field' => 'tid',
      'relationship' => 'none',
      'default_options_div_prefix' => '',
      'default_argument_user' => 0,
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
        'project_project' => 0,
        'project_release' => 0,
        'book' => 0,
        'page' => 0,
        'story' => 0,
      ),
      'validate_argument_node_access' => 0,
      'validate_argument_nid_type' => 'nid',
      'validate_argument_vocabulary' => array(
        '3' => 0,
        '5' => 0,
        '2' => 0,
        '1' => 0,
        '4' => 0,
      ),
      'validate_argument_type' => 'tid',
      'validate_argument_project_term_vocabulary' => array(
        '1' => 1,
      ),
      'validate_argument_project_term_argument_type' => 'convert',
      'validate_argument_project_term_argument_action_top_without' => 'pass',
      'validate_argument_project_term_argument_action_top_with' => 'pass',
      'validate_argument_project_term_argument_action_child' => 'pass',
      'validate_argument_php' => '',
      'override' => array(
        'button' => 'Override',
      ),
    ),
  ));
  $handler->override_option('filters', array(
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
    'status' => array(
      'operator' => '=',
      'value' => 1,
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
    'project_release_api_version' => array(
      'operator' => 'or',
      'value' => array(),
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'project_release_api_version_op',
        'identifier' => 'api',
        'label' => 'API Version',
        'optional' => 1,
        'single' => 1,
        'remember' => 1,
        'reduce' => 0,
      ),
      'type' => 'select',
      'vid' => '2',
      'reduce_duplicates' => TRUE,
      'id' => 'project_release_api_version',
      'table' => 'term_node',
      'field' => 'project_release_api_version',
      'hierarchy' => 0,
      'relationship' => 'project_release_version',
      'override' => array(
        'button' => 'Override',
      ),
    ),
    'keys' => array(
      'operator' => 'optional',
      'value' => '',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'keys_op',
        'identifier' => 'keywords',
        'label' => 'Keywords:',
        'optional' => 1,
        'remember' => 0,
      ),
      'id' => 'keys',
      'table' => 'search_index',
      'field' => 'keys',
      'override' => array(
        'button' => 'Override',
      ),
      'relationship' => 'none',
    ),
    'project_type_tid' => array(
      'operator' => 'or',
      'value' => '',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'project_type_tid_op',
        'identifier' => 'type',
        'label' => 'Category:',
        'optional' => 1,
        'single' => 1,
        'remember' => 0,
        'reduce' => 0,
      ),
      'type' => 'select',
      'vid' => '1',
      'associated_argument' => '0',
      'remove_if_no_options' => 1,
      'argument_position' => '0',
      'id' => 'project_type_tid',
      'table' => 'term_node',
      'field' => 'project_type_tid',
      'relationship' => 'none',
      'reduce_duplicates' => 1,
    ),
    'project_sort_method' => array(
      'value' => 'sticky',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'operator' => '',
        'label' => 'Sort by:',
        'identifier' => 'sort_by',
        'optional' => 0,
        'single' => 1,
        'remember' => 0,
      ),
      'sort_methods' => array(
        'changed' => array(
          'enabled' => 1,
          'weight' => '-23',
          'display_name' => 'Date',
        ),
        'title_1' => array(
          'enabled' => 0,
          'weight' => '-25',
          'display_name' => 'title_1',
        ),
        'sticky' => array(
          'enabled' => 0,
          'weight' => '-24',
          'display_name' => 'sticky',
        ),
        'title' => array(
          'enabled' => 1,
          'weight' => '-22',
          'display_name' => 'Name',
        ),
      ),
      'id' => 'project_sort_method',
      'table' => 'views',
      'field' => 'project_sort_method',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('access', array(
    'type' => 'perm',
    'role' => array(),
    'perm' => 'browse project listings',
  ));
  $handler->override_option('empty', 'No results were found.');
  $handler->override_option('empty_format', '1');
  $handler->override_option('items_per_page', 30);
  $handler->override_option('use_pager', '1');
  $handler->override_option('distinct', 1);
  $handler->override_option('style_plugin', 'project_list');
  $handler->override_option('row_plugin', 'project_fields');
  $handler->override_option('row_options', array(
    'inline' => array(),
    'separator' => '',
  ));
  $handler = $view->new_display('page', 'Page', 'page_1');
  $handler->override_option('path', 'project2/%');
  $handler->override_option('menu', array(
    'type' => 'none',
    'title' => 'Yuki',
    'weight' => '0',
  ));
  $handler->override_option('tab_options', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $handler = $view->new_display('feed', 'Feed', 'feed_1');
  $handler->override_option('style_plugin', 'rss');
  $handler->override_option('style_options', array(
    'mission_description' => FALSE,
    'description' => '',
  ));
  $handler->override_option('row_plugin', 'node_rss');
  $handler->override_option('row_options', array(
    'item_length' => 'default',
  ));
  $handler->override_option('path', 'project2/%/feed');
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
    'default' => 'default',
    'page_1' => 'page_1',
  ));
  $handler->override_option('sitename_title', FALSE);
  $views[$view->name] = $view;

  $view = new view;
  $view->name = 'project_overview';
  $view->description = 'A view for browsing projects.';
  $view->tag = 'project';
  $view->view_php = '';
  $view->base_table = 'node';
  $view->is_cacheable = FALSE;
  $view->api_version = 2;
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->override_option('relationships', array(
    'project_release_version' => array(
      'label' => 'release',
      'required' => 0,
      'id' => 'project_release_version',
      'table' => 'node',
      'field' => 'project_release_version',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('sorts', array(
    'sticky' => array(
      'order' => 'DESC',
      'id' => 'sticky',
      'table' => 'node',
      'field' => 'sticky',
      'relationship' => 'none',
    ),
    'title' => array(
      'order' => 'ASC',
      'id' => 'title',
      'table' => 'node',
      'field' => 'title',
      'relationship' => 'none',
    ),
    'changed' => array(
      'order' => 'DESC',
      'granularity' => 'day',
      'id' => 'changed',
      'table' => 'node',
      'field' => 'changed',
      'relationship' => 'project_release_version',
    ),
    'title_1' => array(
      'order' => 'ASC',
      'id' => 'title_1',
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
      'title' => '%1',
      'default_argument_type' => 'fixed',
      'default_argument' => '',
      'validate_type' => 'project_type_term',
      'validate_fail' => 'not found',
      'break_phrase' => 0,
      'add_table' => 0,
      'require_value' => 0,
      'reduce_duplicates' => 1,
      'set_breadcrumb' => 0,
      'id' => 'tid',
      'table' => 'term_node',
      'field' => 'tid',
      'relationship' => 'none',
      'default_options_div_prefix' => '',
      'default_argument_user' => 0,
      'default_argument_fixed' => '',
      'default_argument_php' => '',
      'validate_argument_node_type' => array(
        'project_project' => 0,
        'project_release' => 0,
        'book' => 0,
        'page' => 0,
        'story' => 0,
      ),
      'validate_argument_node_access' => 0,
      'validate_argument_nid_type' => 'nid',
      'validate_argument_vocabulary' => array(
        '3' => 0,
        '5' => 0,
        '2' => 0,
        '1' => 0,
        '4' => 0,
      ),
      'validate_argument_type' => 'tid',
      'validate_argument_project_term_vocabulary' => array(
        '1' => 1,
      ),
      'validate_argument_project_term_argument_type' => 'convert',
      'validate_argument_project_term_argument_action_top_without' => 'pass',
      'validate_argument_project_term_argument_action_top_with' => 'pass',
      'validate_argument_project_term_argument_action_child' => 'pass',
      'validate_argument_php' => '',
      'override' => array(
        'button' => 'Override',
      ),
    ),
  ));
  $handler->override_option('filters', array(
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
    'status' => array(
      'operator' => '=',
      'value' => 1,
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
    'project_release_api_version' => array(
      'operator' => 'or',
      'value' => array(),
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'project_release_api_version_op',
        'identifier' => 'api',
        'label' => 'API Version',
        'optional' => 1,
        'single' => 1,
        'remember' => 1,
        'reduce' => 0,
      ),
      'type' => 'select',
      'vid' => '2',
      'reduce_duplicates' => TRUE,
      'id' => 'project_release_api_version',
      'table' => 'term_node',
      'field' => 'project_release_api_version',
      'hierarchy' => 0,
      'relationship' => 'project_release_version',
      'override' => array(
        'button' => 'Override',
      ),
    ),
    'keys' => array(
      'operator' => 'optional',
      'value' => '',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'keys_op',
        'identifier' => 'keywords',
        'label' => 'Keywords:',
        'optional' => 1,
        'remember' => 0,
      ),
      'id' => 'keys',
      'table' => 'search_index',
      'field' => 'keys',
      'override' => array(
        'button' => 'Override',
      ),
      'relationship' => 'none',
    ),
    'project_type_tid' => array(
      'operator' => 'or',
      'value' => '',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'use_operator' => 0,
        'operator' => 'project_type_tid_op',
        'identifier' => 'type',
        'label' => 'Category:',
        'optional' => 1,
        'single' => 1,
        'remember' => 0,
        'reduce' => 0,
      ),
      'type' => 'select',
      'vid' => '1',
      'associated_argument' => '0',
      'remove_if_no_options' => 1,
      'argument_position' => '0',
      'id' => 'project_type_tid',
      'table' => 'term_node',
      'field' => 'project_type_tid',
      'relationship' => 'none',
      'reduce_duplicates' => 1,
    ),
    'project_sort_method' => array(
      'value' => 'sticky',
      'group' => '0',
      'exposed' => TRUE,
      'expose' => array(
        'operator' => '',
        'label' => 'Sort by:',
        'identifier' => 'sort_by',
        'optional' => 0,
        'single' => 1,
        'remember' => 0,
      ),
      'sort_methods' => array(
        'changed' => array(
          'enabled' => 1,
          'weight' => '-23',
          'display_name' => 'Date',
        ),
        'title_1' => array(
          'enabled' => 0,
          'weight' => '-25',
          'display_name' => 'title_1',
        ),
        'sticky' => array(
          'enabled' => 0,
          'weight' => '-24',
          'display_name' => 'sticky',
        ),
        'title' => array(
          'enabled' => 1,
          'weight' => '-22',
          'display_name' => 'Name',
        ),
      ),
      'id' => 'project_sort_method',
      'table' => 'views',
      'field' => 'project_sort_method',
      'relationship' => 'none',
    ),
  ));
  $handler->override_option('access', array(
    'type' => 'perm',
    'role' => array(),
    'perm' => 'browse project listings',
  ));
  $handler->override_option('empty', 'No results were found.');
  $handler->override_option('empty_format', '1');
  $handler->override_option('items_per_page', 30);
  $handler->override_option('use_pager', '1');
  $handler->override_option('distinct', 1);
  $handler->override_option('style_plugin', 'project_list');
  $handler->override_option('row_plugin', 'project_node');
  $handler->override_option('row_options', array(
    'project_teaser' => 1,
    'project_term_links' => 1,
    'project_release_download_table' => 1,
    'project_release_download_link' => 1,
  ));
  $handler = $view->new_display('page', 'Page', 'page_1');
  $handler->override_option('path', 'project/%');
  $handler->override_option('menu', array(
    'type' => 'none',
    'title' => 'Yuki',
    'weight' => '0',
  ));
  $handler->override_option('tab_options', array(
    'type' => 'none',
    'title' => '',
    'weight' => 0,
  ));
  $handler = $view->new_display('feed', 'Feed', 'feed_1');
  $handler->override_option('style_plugin', 'rss');
  $handler->override_option('style_options', array(
    'mission_description' => FALSE,
    'description' => '',
  ));
  $handler->override_option('row_plugin', 'node_rss');
  $handler->override_option('row_options', array(
    'item_length' => 'default',
  ));
  $handler->override_option('path', 'project/%/feed');
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
    'default' => 'default',
    'page_1' => 'page_1',
  ));
  $handler->override_option('sitename_title', FALSE);
  $views[$view->name] = $view;

  return $views;
}


