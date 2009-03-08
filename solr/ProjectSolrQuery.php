<?php

include_once drupal_get_path('module', 'apachesolr') .'/Solr_Base_Query.php';

class ProjectSolrQuery extends Solr_Base_Query {
  public $querypath = '';

  function get_url_querystring($include_sort = TRUE) {
    $querystring = '';
    if ($fq = $this->rebuild_fq(TRUE)) {
      $querystring = 'filters='. implode(' ', $fq);
    }
    if ($this->solrsort && $include_sort) {
      $querystring .= ($querystring ? '&' : '') .'solrsort='. $this->solrsort;
    }
    if ($this->querypath) {
      $querystring .= ($querystring ? '&' : '') .'text='. $this->querypath;
    }
    return $querystring;
  }
}
