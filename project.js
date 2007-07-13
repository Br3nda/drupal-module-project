/* $Id: project.js,v 1.2 2007/07/13 21:11:08 dww Exp $ */

Drupal.projectAutoAttach = function () {
  // The initially selected term, if any.
  var tid;
  $('div.project-taxonomy-element input')
    .each(function () {
      if (this.checked) {
        tid = this.value;
      }
    })
    .click(function () {
      Drupal.projectSetTaxonomy(this.value);
    });
  Drupal.projectSetTaxonomy(tid);
}

Drupal.projectSetTaxonomy = function (tid) {
  $('div.project-taxonomy-element select').each(function () {
    // If this is the selector for the currently selected
    // term, show it (in case it was previously hidden).
    if (this.id == 'edit-tid-' + tid) {
      // Hide not the select but its containing div (which also contains
      // the label).
      $(this).parents('div.form-item').show();
    }
    // Otherwise, empty it and hide it.
    else {
      // In case terms were previously selected, unselect them.
      // They are no longer valid.
      this.selectedIndex = -1;
      $(this).parents('div.form-item').hide();
    }
  });
}

// Global killswitch.
if (Drupal.jsEnabled) {
  $(document).ready(Drupal.projectAutoAttach);
}
