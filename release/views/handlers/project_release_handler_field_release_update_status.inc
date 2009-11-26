<?php
// $Id: project_release_handler_field_release_update_status.inc,v 1.1 2009/11/26 22:42:55 dww Exp $

/**
 * Field handler to display the current update status of a release.
 */
class project_release_handler_field_release_update_status extends views_handler_field {
  function option_definition() {
    $options = parent::option_definition();
    $options['update_status_icon'] = array('default' => FALSE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['update_status_icon'] = array(
      '#title' => t('Display as icon'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['update_status_icon']),
      '#description' => t('If checked, the status will be displayed via an icon, not text. See theme_project_release_update_status_icon() for details.'),
    );
  }

  function render($values) {
    if (!empty($this->options['update_status_icon'])) {
      return theme('project_release_update_status_icon', $values->{$this->field_alias});
    }
    else {
      return check_plain(project_release_update_status($values->{$this->field_alias}));
    }
  }
}

