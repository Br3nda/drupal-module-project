<?php

include_once drupal_get_path('module', 'apachesolr') .'/Solr_Base_Query.php';

class ProjectSolrQuery extends Solr_Base_Query {
  /**
   * Return the search path.
   */
  public function get_path() {
    return $this->base_path;
  }
}
