<?php
// $Id: project_release_download_table.view.php,v 1.1 2009/11/30 10:31:32 dww Exp $

$view = new view;
$view->name = 'project_release_download_table';
$view->description = 'Provides tables for the latest release from the recommended branches, the latest releases from any supported branches, and development snapshot releases for a project.';
$view->tag = 'Project release';
$view->view_php = '';
$view->base_table = 'node';
$view->is_cacheable = FALSE;
$view->api_version = 2;
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('relationships', array(
  'supported_releases_rel' => array(
    'label' => 'supported versions',
    'required' => 1,
    'id' => 'supported_releases_rel',
    'table' => 'project_release_nodes',
    'field' => 'supported_releases_rel',
    'relationship' => 'none',
  ),
));
$handler->override_option('fields', array(
  'version' => array(
    'label' => 'Version',
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
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'link_to_node' => 1,
    'exclude' => 0,
    'id' => 'version',
    'table' => 'project_release_nodes',
    'field' => 'version',
    'relationship' => 'none',
  ),
  'files' => array(
    'label' => 'Downloads',
    'alter' => array(
      'alter_text' => 1,
      'text' => 'Download <span class="filesize">([files-size])</span>',
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
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'type' => 'separator',
    'separator' => ' | ',
    'link_to_file' => 1,
    'max_files' => '1',
    'file_sort' => 'fid',
    'file_sort_order' => 'DESC',
    'exclude' => 0,
    'id' => 'files',
    'table' => 'project_release_nodes',
    'field' => 'files',
    'relationship' => 'none',
  ),
  'changed' => array(
    'label' => 'Date',
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
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'date_format' => 'custom',
    'custom_date_format' => 'Y-M-d',
    'exclude' => 0,
    'id' => 'changed',
    'table' => 'node',
    'field' => 'changed',
    'relationship' => 'none',
  ),
  'view_node' => array(
    'label' => 'Links',
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
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'text' => 'Notes',
    'exclude' => 0,
    'id' => 'view_node',
    'table' => 'node',
    'field' => 'view_node',
    'relationship' => 'none',
  ),
  'edit_node' => array(
    'label' => 'Edit link',
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
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'text' => 'Edit',
    'exclude' => 0,
    'id' => 'edit_node',
    'table' => 'node',
    'field' => 'edit_node',
    'relationship' => 'none',
  ),
));
$handler->override_option('sorts', array(
  'version_major' => array(
    'order' => 'DESC',
    'id' => 'version_major',
    'table' => 'project_release_nodes',
    'field' => 'version_major',
    'relationship' => 'none',
  ),
  'version_minor' => array(
    'order' => 'DESC',
    'id' => 'version_minor',
    'table' => 'project_release_nodes',
    'field' => 'version_minor',
    'relationship' => 'none',
  ),
  'version_patch' => array(
    'order' => 'DESC',
    'id' => 'version_patch',
    'table' => 'project_release_nodes',
    'field' => 'version_patch',
    'relationship' => 'none',
  ),
  'version_extra_weight' => array(
    'order' => 'DESC',
    'id' => 'version_extra_weight',
    'table' => 'project_release_nodes',
    'field' => 'version_extra_weight',
    'relationship' => 'none',
  ),
  'version_extra' => array(
    'order' => 'DESC',
    'id' => 'version_extra',
    'table' => 'project_release_nodes',
    'field' => 'version_extra',
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
    ),
    'validate_argument_type' => 'tid',
    'validate_argument_php' => '',
    'validate_argument_node_access' => 1,
    'validate_argument_nid_type' => 'nid',
    'default_options_div_prefix' => '',
    'validate_argument_project_term_vocabulary' => array(),
  ),
));
$handler->override_option('filters', array(
  'status_extra' => array(
    'operator' => '=',
    'value' => '',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status_extra',
    'table' => 'node',
    'field' => 'status_extra',
    'relationship' => 'none',
  ),
  'version_api_tid' => array(
    'operator' => 'in',
    'value' => array(),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => 'version_api_tid_op',
      'label' => 'Project release: API compatibility term',
      'use_operator' => FALSE,
      'identifier' => 'version_api_tid',
      'remember' => FALSE,
      'single' => TRUE,
      'optional' => TRUE,
      'reduce' => FALSE,
    ),
    'type' => 'all',
    'id' => 'version_api_tid',
    'table' => 'project_release_nodes',
    'field' => 'version_api_tid',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'supported' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'supported',
    'table' => 'project_release_supported_versions',
    'field' => 'supported',
    'relationship' => 'supported_releases_rel',
  ),
  'rebuild' => array(
    'operator' => '=',
    'value' => '0',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'rebuild',
    'table' => 'project_release_nodes',
    'field' => 'rebuild',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
));
$handler->override_option('access', array(
  'type' => 'none',
  'role' => array(),
  'perm' => 'access projects',
));
$handler->override_option('cache', array(
  'type' => 'none',
));
$handler->override_option('header_empty', 0);
$handler->override_option('empty_format', '1');
$handler->override_option('items_per_page', 0);
$handler->override_option('use_pager', '0');
$handler->override_option('style_plugin', 'table');
$handler->override_option('style_options', array(
  'grouping' => '',
  'override' => 1,
  'sticky' => 0,
  'order' => 'desc',
  'columns' => array(
    'version' => 'version',
    'files' => 'files',
    'changed' => 'changed',
    'view_node' => 'view_node',
    'edit_node' => 'view_node',
  ),
  'info' => array(
    'version' => array(
      'sortable' => 0,
      'separator' => '',
    ),
    'files' => array(
      'separator' => '',
    ),
    'changed' => array(
      'sortable' => 0,
      'separator' => '',
    ),
    'view_node' => array(
      'separator' => ' | ',
    ),
    'edit_node' => array(
      'separator' => '',
    ),
  ),
  'default' => 'version',
));
$handler->override_option('row_options', array(
  'inline' => array(),
  'separator' => '',
));
$handler = $view->new_display('attachment', 'Recommended branches', 'attachment_1');
$handler->override_option('filters', array(
  'status_extra' => array(
    'operator' => '=',
    'value' => '',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status_extra',
    'table' => 'node',
    'field' => 'status_extra',
    'relationship' => 'none',
  ),
  'version_api_tid' => array(
    'operator' => 'in',
    'value' => array(),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => 'version_api_tid_op',
      'label' => 'Project release: API compatibility term',
      'use_operator' => FALSE,
      'identifier' => 'version_api_tid',
      'remember' => FALSE,
      'single' => TRUE,
      'optional' => TRUE,
      'reduce' => FALSE,
    ),
    'type' => 'all',
    'id' => 'version_api_tid',
    'table' => 'project_release_nodes',
    'field' => 'version_api_tid',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'supported' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'supported',
    'table' => 'project_release_supported_versions',
    'field' => 'supported',
    'relationship' => 'supported_releases_rel',
  ),
  'rebuild' => array(
    'operator' => '=',
    'value' => '0',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'rebuild',
    'table' => 'project_release_nodes',
    'field' => 'rebuild',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'release_type' => array(
    'operator' => '=',
    'value' => 'recommended',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'release_relationship' => 'base',
    'id' => 'release_type',
    'table' => 'project_release_supported_versions',
    'field' => 'release_type',
    'relationship' => 'supported_releases_rel',
    'override' => array(
      'button' => 'Use default',
    ),
  ),
  'recommended' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'recommended',
    'table' => 'project_release_supported_versions',
    'field' => 'recommended',
    'relationship' => 'supported_releases_rel',
    'override' => array(
      'button' => 'Use default',
    ),
  ),
));
$handler->override_option('attachment_position', 'before');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', FALSE);
$handler->override_option('displays', array());
$handler = $view->new_display('attachment', 'Supported branches', 'attachment_2');
$handler->override_option('filters', array(
  'status_extra' => array(
    'operator' => '=',
    'value' => '',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status_extra',
    'table' => 'node',
    'field' => 'status_extra',
    'relationship' => 'none',
  ),
  'version_api_tid' => array(
    'operator' => 'in',
    'value' => array(),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => 'version_api_tid_op',
      'label' => 'Project release: API compatibility term',
      'use_operator' => FALSE,
      'identifier' => 'version_api_tid',
      'remember' => FALSE,
      'single' => TRUE,
      'optional' => TRUE,
      'reduce' => FALSE,
    ),
    'type' => 'all',
    'id' => 'version_api_tid',
    'table' => 'project_release_nodes',
    'field' => 'version_api_tid',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'supported' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'supported',
    'table' => 'project_release_supported_versions',
    'field' => 'supported',
    'relationship' => 'supported_releases_rel',
  ),
  'rebuild' => array(
    'operator' => '=',
    'value' => '0',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'rebuild',
    'table' => 'project_release_nodes',
    'field' => 'rebuild',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'release_type' => array(
    'operator' => '=',
    'value' => 'recommended',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'release_relationship' => 'base',
    'id' => 'release_type',
    'table' => 'project_release_supported_versions',
    'field' => 'release_type',
    'relationship' => 'supported_releases_rel',
  ),
  'recommended' => array(
    'operator' => '=',
    'value' => '0',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'recommended',
    'table' => 'project_release_supported_versions',
    'field' => 'recommended',
    'relationship' => 'supported_releases_rel',
    'override' => array(
      'button' => 'Use default',
    ),
  ),
));
$handler->override_option('attachment_position', 'before');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', FALSE);
$handler->override_option('displays', array());
$handler = $view->new_display('attachment', 'Development snapshots', 'attachment_3');
$handler->override_option('filters', array(
  'status_extra' => array(
    'operator' => '=',
    'value' => '',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'status_extra',
    'table' => 'node',
    'field' => 'status_extra',
    'relationship' => 'none',
  ),
  'version_api_tid' => array(
    'operator' => 'in',
    'value' => array(),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => 'version_api_tid_op',
      'label' => 'Project release: API compatibility term',
      'use_operator' => FALSE,
      'identifier' => 'version_api_tid',
      'remember' => FALSE,
      'single' => TRUE,
      'optional' => TRUE,
      'reduce' => FALSE,
    ),
    'type' => 'all',
    'id' => 'version_api_tid',
    'table' => 'project_release_nodes',
    'field' => 'version_api_tid',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'supported' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'supported',
    'table' => 'project_release_supported_versions',
    'field' => 'supported',
    'relationship' => 'supported_releases_rel',
  ),
  'rebuild' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'rebuild',
    'table' => 'project_release_nodes',
    'field' => 'rebuild',
    'override' => array(
      'button' => 'Use default',
    ),
    'relationship' => 'none',
  ),
  'snapshot' => array(
    'operator' => '=',
    'value' => '1',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'snapshot',
    'table' => 'project_release_supported_versions',
    'field' => 'snapshot',
    'relationship' => 'supported_releases_rel',
    'override' => array(
      'button' => 'Use default',
    ),
  ),
));
$handler->override_option('attachment_position', 'before');
$handler->override_option('inherit_arguments', TRUE);
$handler->override_option('inherit_exposed_filters', FALSE);
$handler->override_option('displays', array());

