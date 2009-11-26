<?php
// $Id: project_release.views.inc,v 1.6 2009/11/26 04:31:24 dww Exp $

/**
 * @file
 * Provide views data and handlers for project_release.module.
 */

/**
 * Implementation of hook_views_data().
 *
 * Exposes all fields to the views system.
 */
function project_release_views_data() {
  $data = array();
  // ----------------------------------------------------------------
  // project_projectss table

  // Define the base group of this table. Fields that don't
  // have a group defined will go into this field by default.
  $data['project_release_nodes']['table']['group']  = t('Project release');

  $data['project_release_nodes']['table']['join'] = array(
    'node' => array(
      'left_field' => 'nid',
      'field' => 'nid',
    ),
  );

  // nid
  $data['project_release_nodes']['nid'] = array(
    'title' => t('Release node'),
    'help' => t('The node ID of the release node.'),
    'relationship' => array(
      'help' => t('Add a relationship to gain access to more node data for releases.'),
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Release node'),
    ),
  );

  // pid
  $data['project_release_nodes']['pid'] = array(
    'title' => t('Pid'),
    'help' => t('The node ID of the project node a release is tied to.'),
    'field' => array(
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
    ),
    // Information for accepting a pid as an argument
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'name field' => 'title',
      'numeric' => TRUE,
      'validate type' => 'nid',
    ),
    // Information for accepting a pid as a filter
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'relationship' => array(
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Project node'),
    ),
  );
  // Add a relationship for information and filters about supported releases.
  $data['project_release_nodes']['supported_releases_rel'] = array(
    'group' => t('Project release'),
    'title' => t('Supported releases'),
    'help' => t('Create a relationship to more information about supported releases.'),
    'real field' => 'pid',
    'relationship' => array(
      'base' => 'project_release_supported_versions',
      'relationship field' => 'pid',
      'handler' => 'project_release_handler_relationship_supported_versions',
      'label' => t('supported versions'),
    ),
  );

  // version
  $data['project_release_nodes']['version'] = array(
    'title' => t('Version string'),
    'help' => t('The full version string of a release.'),
    'field' => array(
      'group' => t('Project release'),
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
     ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );

  // tag
  $data['project_release_nodes']['tag'] = array(
    'title' => t('Tag'),
    'help' => t('The tag associated with a release.'),
    'field' => array(
      'group' => t('Project release'),
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
     ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );

  $data['project_release_nodes']['files'] = array(
    'title' => t('Project release files'),
    'help' => t('All files attached to a release node.'),
    'real field' => 'nid',
    'field' => array(
      'handler' => 'project_release_handler_field_files',
    ),
    // @TODO: filter
    'relationship' => array(
      'title' => t('Release files'),
      'help' => t('Add a relationship to gain access to more file data for files attached to release nodes. Note that this relationship will cause duplicate nodes if there are multiple files attached to the release.'),
      'relationship table' => 'project_release_file',
      'relationship field' => 'fid',
      'base' => 'files',
      'field' => 'fid',
      'handler' => 'views_handler_relationship',
      'label' => t('Release files'),
    ),
  );

  // @TODO:  rebuild
  // @TODO:  version_major
  // @TODO:  version_minor
  // @TODO:  version_patch
  // @TODO:  version_extra
  // @TODO:  version_extra_weight

  $data['project_release_file']['table']['group']  = t('Project release');
  $data['project_release_file']['table']['join'] = array(
    'node' => array(
      'left_field' => 'nid',
      'field' => 'nid',
    ),
    'files' => array(
      'left_field' => 'fid',
      'field' => 'fid',
    ),
  );

  $data['project_release_supported_versions']['table']['group']  = t('Project release');
  // We only want to JOIN on this table via explicit relationships.

  $data['project_release_supported_versions']['nid'] = array(
    'title' => t('Project ID'),
    'help' => t('Project node ID of the supported version information.'),
    'field' => array(
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'name field' => 'title',
      'numeric' => TRUE,
      'validate type' => 'nid',
    ),
    'relationship' => array(
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Project node'),
    ),
  );
  $data['project_release_supported_versions']['supported'] = array(
    'title' => t('Supported'),
    'help' => t('Is the specific branch supported by a given project.'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'type' => 'yes-no',
    ),
  );
  $data['project_release_supported_versions']['recommended'] = array(
    'title' => t('Recommended branch'),
    'help' => t('Is the specific branch the recommended branch for a given project.'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'type' => 'yes-no',
    ),
  );
  $data['project_release_supported_versions']['snapshot'] = array(
    'title' => t('Show snapshot'),
    'help' => t('Show development snapshot releases from a branch.'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'type' => 'yes-no',
    ),
  );
  $data['project_release_supported_versions']['recommended_release'] = array(
    'title' => t('Recommended release'),
    'help' => t('The recommended release from a specific branch of a project.'),
    'field' => array(
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'name field' => 'title',
      'numeric' => TRUE,
      'validate type' => 'nid',
    ),
    'relationship' => array(
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Recommended release node'),
    ),
  );
  $data['project_release_supported_versions']['latest_release'] = array(
    'title' => t('Latest release'),
    'help' => t('The latest release from a specific branch of a project.'),
    'field' => array(
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'name field' => 'title',
      'numeric' => TRUE,
      'validate type' => 'nid',
    ),
    'relationship' => array(
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Latest release node'),
    ),
  );
  $data['project_release_supported_versions']['latest_security_release'] = array(
    'title' => t('Latest security release'),
    'help' => t('The latest security release from a specific branch of a project (if any).'),
    'field' => array(
      'handler' => 'views_handler_field_node',
      'click sortable' => TRUE,
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'name field' => 'title',
      'numeric' => TRUE,
      'validate type' => 'nid',
    ),
    'relationship' => array(
      'base' => 'node',
      'handler' => 'views_handler_relationship',
      'label' => t('Latest security release node'),
    ),
  );
  $data['project_release_supported_versions']['release_type'] = array(
    'title' => t('Latest or recommended release'),
    'help' => t('Filter based on the latest or recommended release from a specific branch of a project.'),
    'filter' => array(
      'handler' => 'project_release_handler_filter_release_node_nid',
    ),
  );

  return $data;
}

/**
 * Implementation of hook_views_data_alter().
 */
function project_release_views_data_alter(&$data) {
  // Project release version relationship.
  $data['node']['project_release_version'] = array(
    'group' => t('Project release'),
    'title' => t('Version'),
    'help' => t('Create a relationship from a project node to a project release node.'),
    'real field' => 'nid',
    'relationship' => array(
      'handler' => 'project_release_handler_relationship_project_project_release_node',
      'base' => 'node',
      'field' => 'nid',
      'label' => t('release'),
    ),
  );

  // Add a relationship for releases associated with a given project.
  $data['node']['project_release_rel'] = array(
    'group' => t('Project'),
    'title' => t('Releases'),
    'help' => t('Create a relationship to more information about all releases for a project.'),
    'real field' => 'nid',
    'relationship' => array(
      'base' => 'project_release_nodes',
      'base field' => 'pid',
      'relationship field' => 'nid',
      'handler' => 'views_handler_relationship',
      'label' => t('releases'),
    ),
  );

  // Add a relationship for information and filters about supported releases.
  $data['project_projects']['supported_releases_rel'] = array(
    'group' => t('Project'),
    'title' => t('Supported releases'),
    'help' => t('Create a relationship to more information about supported releases.'),
    'real field' => 'nid',
    'relationship' => array(
      'base' => 'project_release_supported_versions',
      'base field' => 'nid',
      'relationship field' => 'nid',
      'handler' => 'views_handler_relationship',
      'label' => t('supported versions'),
    ),
  );

  /**
   * Special fields
   *
   * These fields are not not stored in the tables created by project_release
   * and are provided as a way to provide information that would be unavailable
   * using other fields provided by this module or by views itself.
   */
  // changed field
  $data['node']['project_release_most_recent_release'] = array(
    'group' => t('Project release'),
    'title' => t('Most recent release changed time'),
    'help' => t('The date the most recent project release node associated with a project was last updated.'),
    'real field' => 'changed',
    'field' => array(
      'handler' => 'project_release_handler_field_most_recent_release',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'project_release_handler_sort_most_recent_release',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date', // @TODO
    ),
  );

  // API version compatability term field.
  // We could just use the Taxonomy: Term ID in taxonomy.views.inc except
  // that we want to only display terms the admin has indicated as being
  // active on the Project release module settings page.
  $data['term_node']['project_release_api_version'] = array(
    'group' => t('Project release'),
    'title' => t('API compatability term'),
    'help' => t('The project release API compatability term (version).'),
    'field' => array(
      'title' => t('All terms'),
      'help' => t('Display all taxonomy terms associated with a node.'),
      'handler' => 'views_handler_field_term_node_tid',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_term_node_tid',
      'name table' => 'term_data',
      'name field' => 'name',
      'empty name field' => t('Uncategorized'),
      'numeric' => TRUE,
    ),
    'filter' => array(
      'field' => 'tid',
      'handler' => 'project_release_handler_filter_project_release_api_version',
      'hierarchy table' => 'term_hierarchy',
      'numeric' => TRUE,
    ),
  );

  // Project download table field
  $data['node']['download_table'] = array(
    'group' => t('Project release'),
    'field' => array(
      'title' => t('Download table'),
      'help' => t('A project release download table for the project.'),
      'handler' => 'project_release_handler_field_download_table',
      'format' => 'format',
    ),
  );
}

/**
 * Implementation of hook_views_handlers().
 */
function project_release_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'project_release') .'/views/handlers',
    ),
    'handlers' => array(
      'project_release_handler_field_download_table' => array(
        'parent' => 'views_handler_field',
      ),
      'project_release_handler_field_files' => array(
        'parent' => 'views_handler_field_prerender_list',
      ),
      'project_release_handler_field_most_recent_release' => array(
        'parent' => 'views_handler_field_date',
      ),
      'project_release_handler_filter_project_release_api_version' => array(
        'parent' => 'views_handler_filter_term_node_tid',
      ),
      'project_release_handler_filter_release_node_nid' => array(
        'parent' => 'views_handler_filter',
      ),
      'project_release_handler_relationship_project_project_release_node' => array(
        'parent' => 'views_handler_relationship',
      ),
      'project_release_handler_relationship_supported_versions' => array(
        'parent' => 'views_handler_relationship',
      ),
      'project_release_handler_sort_most_recent_release' => array(
        'parent' => 'views_handler_sort_date',
      ),
    ),
  );
}

