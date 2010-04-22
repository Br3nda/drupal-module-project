/* $Id: project.js,v 1.4 2010/04/22 06:10:24 thehunmonkgroup Exp $ */

Drupal.behaviors.projectAuto = function (context) {
  // The initially selected term, if any.
  var tid;
  $('div.project-taxonomy-element input:not(.projectAuto-processed)', context).addClass('projectAuto-processed').each(function () {
      if (this.checked) {
        tid = this.value;
      }
      Drupal.projectMoveElement(this.value);
    })
    .click(function () {
      Drupal.projectSetTaxonomy(this.value);
    });
  Drupal.projectSetTaxonomy(tid);
}

Drupal.projectMoveElement = function(tid) {
  // move all elements with a class linked to this tid into the
  // project taxonomy fieldset (similar to module sub-terms)
  $('.related-tid-' + tid).each(function() {
    $('#edit-tid-' + tid + '-wrapper').append($(this).parent().remove());
  }); 
}

Drupal.projectSetTaxonomy = function (tid) {
  $('div.project-taxonomy-element select').each(function () {
    // If this is the selector for the currently selected term or a
    // related element, show it (in case it was previously hidden).
    if (this.id == 'edit-tid-' + tid || $(this).hasClass('related-tid-' + tid)) {
      // Hide not the select but its containing div (which also contains
      // the label).
      $(this).parents('div.form-item').show();
    }
    // Otherwise, hide it.
    else {
      $(this).parents('div.form-item').hide();
    }
  });
}
