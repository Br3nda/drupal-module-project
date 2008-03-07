/* $Id: project_release.js,v 1.2 2008/03/07 19:35:08 dww Exp $ */

Drupal.projectReleaseAutoAttach = function () {
  // Set handler for clicking a radio to change the recommended version.
  $('form#project-release-project-edit-form input.form-radio.recommended').click(function () {
    Drupal.projectReleaseSetRecommended( this );
  });
  
  // Set handler for clicking checkbox to toggle a version supported/unsupported.
  $('form#project-release-project-edit-form input.form-checkbox.supported').click(function() {
    if (this.checked) {
      // Marking this version as supported.
      $(this).parents('tr:eq(0)').find('.recommended, .snapshot').removeAttr('disabled');
      // If there are no recommended versions, make this newly supported version recommended.
      if (!Drupal.projectReleaseIsRecommendedSet($(this).parents('table:eq(0)'))) {
        Drupal.projectReleaseSetRecommended($(this).parents('tr:eq(0)').find('.recommended'));
      }
    }
    else {
      // Marking this version as unsupported, so disable row.
      $(this).parents('tr:eq(0)').find('.recommended, .snapshot')
        .attr('disabled','true')
        .removeAttr('checked');

      // Handle case were there are now no recommended versions.
      if (!Drupal.projectReleaseIsRecommendedSet($(this).parents('table:eq(0)'))) {
        // See if there is at least one supported versions.
        var recommendable = null;
        $(this).parents('table:eq(0)').find(".recommended").each( function(i) {
          if (!this.disabled) {
            recommendable = this;
          }
        });
        if (recommendable) {
          // There is a supported version, so recommend it.
          Drupal.projectReleaseSetRecommended( recommendable );
        }
        else {
          // There are no supported versions.
          Drupal.projectReleaseUnsetRecommended( $(this).parents('table:eq(0)') );
        }
      }
    }
  }).each( function() { // Disable unsupported versions on initial page load.
    if (!this.checked) {
      $(this).parents('tr:eq(0)').find('.recommended, .snapshot').attr('disabled','true');
    }
  });
};

Drupal.projectReleaseIsRecommendedSet = function (table) {
  var recommended = false;
  $(table).find(".recommended").each( function(i) {
    if (this.checked) {
      recommended = true;
    }
  });
  return recommended;
};

Drupal.projectReleaseSetRecommended = function (radio) {
  $(radio).attr('checked','true');
  var recommended = $(radio).parents('tr:eq(0)').find('.version-name').val();
  $(radio).parents('table:eq(0)').find('tr:last span')
    .html(recommended)
    .css('background-color', '#FFFFAA');
};

Drupal.projectReleaseUnsetRecommended = function (table) {
    $(table).find('tr:last span')
      .html('n/a')
      .css('background-color', '#FFFFAA');
};

// Global killswitch.
if (Drupal.jsEnabled) {
  $(document).ready( Drupal.projectReleaseAutoAttach);
}
