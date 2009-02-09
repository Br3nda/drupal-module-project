<?php

include_once drupal_get_path('module', 'apachesolr') .'/Solr_Base_Query.php';

class ProjectSolrQuery extends Solr_Base_Query {
  public $querypath = '';

  function get_url_querystring() {
    $querystring = parent::get_url_querystring();
    if ($this->querypath) {
      $querystring .= ($querystring ? '&' : '') .'text='. $this->querypath;
    }
    return $querystring;
  }
}
