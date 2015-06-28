/**
 * Licensed under the MIT license:
 *   http://www.opensource.org/licenses/mit-license.php
 *
 * To use this, you need to include thermometer.js, jquery, and this file:
 *
 *   <script type="text/javascript" src="../jquery-1.6.min.js"></script>
 *   <script type="text/javascript" src="../thermometer.js"></script>
 *   <script type="text/javascript" src="../jquery.thermometer.js"></script>
 *
 *
 */
(function( $ ){
  var methods = {
    init : function( options ) {
   
        return this.each(function(){
            var therm = new ThermometerGuage( $(this)[0], options );
            $(this).data('therm', therm);
        });    
       
    },
    setValue : function( value ) {
        return this.each(function(){
            var therm = $(this).data('therm');
            if (therm != null) {        
                therm.setValue( value );
            }
        });
    } 
  };

  $.fn.thermometer = function( method ) {
   
    // Method calling logic
    if ( methods[method] ) {
      return methods[ method ].apply( this, Array.prototype.slice.call( arguments, 1 ));
    } else if ( typeof method === 'object' || ! method ) {
      return methods.init.apply( this, arguments );
    } else {
      $.error( 'Method ' +  method + ' does not exist on jQuery.thermometer' );
    }    
 
  };
})( jQuery );
