<?php
// $Id $

/**
 * This is a temporary workaround around the current sad state
 * of Solr_Base_Query. We need to define non-indexed filters.
 */

class Project_Solr_Query extends Solr_Base_Query {

  protected function parse_query() {
    $this->fields = array();
    $filters = $this->filters;

    // Gets information about the fields already in solr index.
    $index_fields = $this->solr->getFields();

    // XXX: this is the specific part: add our fields.
    $index_fields->core = TRUE;

    $rows = array();
    foreach ((array) $index_fields as $name => $field) {
      do {
        // save the strlen so we can detect if it has changed at the bottom
        // of the do loop
        $a = (int)strlen($filters);
        // Get the values for $name
        $extracted = Solr_Base_Query::query_extract($filters, $name);
        if (count($extracted['values'])) {
          foreach ($extracted['values'] as $value) {
            $found = Solr_Base_Query::make_field(array('#name' => $name, '#value' => $value));
            $pos = strpos($this->filters, $found);
            // $solr_keys and $solr_crumbs are keyed on $pos so that query order
            // is maintained. This is important for breadcrumbs.
            $this->fields[$pos] = array('#name' => $name, '#value' => trim($value));
          }
          // Update the local copy of $filters by removing the key that was just found.
          $filters = trim(str_replace($extracted['matches'], '', $filters));
        }
        // Take new strlen to compare with $a.
        $b = (int)strlen($filters);
      } while ($a !== $b);
    }
    // Even though the array has the right keys they are likely in the wrong
    // order. ksort() sorts the array by key while maintaining the key.
    ksort($this->fields);
  }
}
