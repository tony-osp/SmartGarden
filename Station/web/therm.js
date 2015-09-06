function ThermometerGuage(canvas, options) {
    var that = this;
    this.ctx = canvas.getContext('2d');
    this.settings = {
        w: canvas.width,
        h: canvas.height,
        ctx: this.ctx,
        bulbRadius: function() {
                return ((this.bulbRadiusByHeight || false) ? this.h : this.w) * this.bulbRadiusProportion;
            },
        color: {}
    };

    function setValue(val) {
        var max = that.settings.max;
        var min = that.settings.min;

        that.settings.fillToInput = val;
        if (val > max) 
          that.settings.fillTo = 1;
        else if (val < min) 
          that.settings.fillTo = 0;
        else
          that.settings.fillTo = ((val - min) / (max - min));

        clearCanvas();
        drawThermometerContainer();
        redrawFill();
    };

    function getFillTo() {
        return that.settings.fillToInput;
    };

    function fillText(context, text, px, py) {
        var width;
        if (context.fillText) {
            return context.fillText(text, px, py);
        } else if (context.mozDrawText) { //FF < 3.5
            context.save();
            context.translate(px, py);
            width = context.mozDrawText(text);
            context.restore();
            return width;
        }
        throw "fillText() not supported!";
    };

    function clearCanvas() {
        that.ctx.clearRect(0, 0, that.settings.w, that.settings.h);
    };

    function setOptions(options) {
        var _ = that.settings;
        //jQuery.extend(true, that.settings, options);  //the easy way, but depends on jQuery, so here's the hard way ;-(  ...
            options.color = options.color || {};
            _.w                     =  options.w                     || _.w;
            _.h                     =  options.h                     || _.h;
            _.ctx                   =  options.ctx                   || _.ctx;
            _.max                   =  options.max                   || _.max                    ||  1;
            _.min                   =  options.min                   || _.min                    ||  0;
            _.gradient              =  options.gradient              || _.gradient               ||  false;
            _.opacity               =  options.opacity               || _.opacity                ||  180;
            _.debug                 =  options.debug                 || _.debug                  ||  false;
            _.color.fill            =  options.color.fill            || _.color.fill             ||  '#ff0000';
            _.color.label           =  options.color.label           || _.color.label            ||  'rgba(45, 45, 45, 1)';
            _.color.outline         =  options.color.outline         || _.color.outline          ||  'rgba(0, 0, 0, 0.5)';
            _.color.outlineFill     =  options.color.outlineFill     || _.color.outlineFill      ||  'rgba(0, 128, 255, 0.2)';
            _.color.tick            =  options.color.tick            || _.color.tick             ||  'black';
            _.color.tickLabel       =  options.color.tickLabel       || _.color.tickLabel        ||  'rgba(45, 45, 45, 1)';
            _.bulbRadiusProportion  =  options.bulbRadiusProportion  || _.bulbRadiusProportion   ||  0.1;
            _.bulbRadiusByHeight    =  options.bulbRadiusByHeight    || _.bulbRadiusByHeight     ||  false;
            _.majorTicks            =  options.majorTicks            || _.majorTicks             ||  5;
            _.minorTicks            =  options.minorTicks            || _.minorTicks             ||  4;
            _.centerTicks           =  options.centerTicks           || _.centerTicks            ||  false;
            _.scaleTickLabelText    =  options.scaleTickLabelText    || _.scaleTickLabelText     ||  1;
            _.scaleLabelText        =  options.scaleLabelText        || _.scaleLabelText         ||  1;
            _.scaleTickWidth        =  options.scaleTickWidth        || _.scaleTickWidth         ||  1;
            _.unitsLabel            =  options.unitsLabel            || _.unitsLabel             ||  "";

        var h = _.h;
        var bR = _.bulbRadius(); //20
        var tR = _.topRadius = Math.cos(Math.PI * 1.6) * bR;
        var bOR = _.bulbOutsideRadius = bR * 1.3; //26
        var bVC = _.bulbVerticalCenter = h - bOR - 1; 
        var tVC = _.topVerticalCenter = bR * 0.3 + bOR - bR + 1;
        _.innerHeight = bVC - tVC + bR + Math.cos(Math.PI * 1.6) * bR;
        _.topOfBulb = bVC + Math.sin(Math.PI * 1.4) * bR;
        _.topOfOutsideBulb = bVC + Math.sin(Math.PI * 1.35) * bOR;
        _.bottomOfBulb = bVC + bR;
        _.innerTop = tVC - tR;
        _.bulbPercent = (_.bottomOfBulb - _.topOfBulb) / _.innerHeight;
    };

    function drawThermometerContainer() {
      var _ = that.settings;
      var ctx = that.ctx;
      var w = _.w;
      var h = _.h;
      
      //var bulbRadius = bR = config.bulbRadius = ((w<h)?w:h)*0.1; //20
      var bR = _.bulbRadius(); //20
      var tR = _.topRadius;
      var bOR = _.bulbOutsideRadius; //26
      var bVC = _.bulbVerticalCenter; // (padding) // This was 160 before making derived;
      var tVC = _.topVerticalCenter;
      
      ctx.save();
      ctx.strokeStyle = _.color.outline;
      ctx.fillStyle = _.color.outlineFill;
      
      //Draw the outer thermometer outline
      ctx.beginPath();
      ctx.arc(w / 2, bVC, bOR, Math.PI * 1.35, Math.PI * 1.65, true); // Outer circle  
      ctx.lineTo(w / 2 + Math.cos(Math.PI * 1.65) * bOR, tVC);
      ctx.arc(w / 2, tVC, Math.cos(Math.PI * 1.65) * bOR, 0, Math.PI, true);
      ctx.lineTo(w / 2 + Math.cos(Math.PI * 1.35) * bOR, bVC + Math.sin(Math.PI * 1.35) * bOR);
      
      ctx.closePath();
      
      ctx.stroke();
      ctx.fill();
      
      //Draw the inner thermometer outline
      ctx.fillStyle = "rgba(255,255,255,1)";
      ctx.beginPath();
      
      ctx.moveTo(w / 2 + Math.cos(Math.PI * 1.4) * bR, bVC + Math.sin(Math.PI * 1.4) * bR);
      ctx.arc(w / 2, bVC, bR, Math.PI * 1.4, Math.PI * 1.6, true); // Outer circle  
      ctx.lineTo(w / 2 + Math.cos(Math.PI * 1.6) * bR, tVC);
      ctx.arc(w / 2, tVC, Math.cos(Math.PI * 1.6) * bR, 0, Math.PI, true);
      ctx.lineTo(w / 2 + Math.cos(Math.PI * 1.4) * bR, bVC + Math.sin(Math.PI * 1.4) * bR);
      
      ctx.closePath();
      
      ctx.globalCompositeOperation = 'destination - out';
      ctx.fill();
      ctx.globalCompositeOperation = 'source - over';
      ctx.stroke();
      ctx.restore();

    };

    function drawTick(y, centerTicks, lineWidth, labelText) {
        var ctx = that.ctx;
        var _ = that.settings;
        var w = _.w;
        var bOR = _.bulbOutsideRadius;
        var scaleFactor = _.scaleTickWidth;
        var tickWidth = _.bulbRadius() - _.bulbOutsideRadius;

        //Make sure the ticks are on half pixels so the lines are rendered fine.
        cy = _.innerTop + _.innerHeight * y;
        cy = pushToHalf(cy);    //Adjust so that the value is rounded to the nearest half (like 1.5, 2.5, etc, not whole numbers)
        var angleHeight = _.bottomOfBulb - cy - _.bulbVerticalCenter;
        if (_.centerTicks) {
            cx = w / 2 - tickWidth / 2;
        } else if (cy > _.topOfOutsideBulb) {
            cx = w / 2 - Math.cos(Math.asin((cy - _.bulbVerticalCenter) / bOR)) * bOR - bOR * 0.1;
        } else {
            cx = w / 2 + Math.cos(Math.PI * 1.35) * bOR - bOR * 0.1;
        }
        ctx.save();
        ctx.strokeStyle = _.color.tick;
        ctx.lineWidth = lineWidth;
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(cx + (tickWidth * scaleFactor), cy);

        ctx.closePath();

        ctx.stroke();
        if (labelText != null) {
          drawTickLabel(cx + (tickWidth * scaleFactor) * 2 , cy, labelText, -tickWidth);
        }
        ctx.restore();

    }

    function pushToHalf(value) {
        var converted = parseFloat(value); // Make sure we have a number
        var decimal = (converted - parseInt(converted, 10));
        decimal = Math.round(decimal * 10);
        return (parseInt(converted, 10) + 0.5);
    }

    function redrawFill() {
      var _ = that.settings;
      var ctx = that.ctx;
      var w = _.w;
      var h = _.h;
      var bR = _.bulbRadius();
      var bVC = _.bulbVerticalCenter;
      var tVC = _.topVerticalCenter;
      var fillTo = _.fillTo;

      ctx.strokeStyle = _.color.fill;
      ctx.fillStyle = _.color.fill;
      
      if (fillTo > _.bulbPercent) { //easy way, fill the whole bulb
          ctx.beginPath();
          ctx.arc(w / 2, bVC, bR, Math.PI * 1.4, Math.PI * 1.6, true); // Outer circle  
          ctx.closePath();
          ctx.fill();
          var fillHeight = fillTo * _.innerHeight;
          var fillTop = _.bottomOfBulb - fillHeight;
          //Now, determine if the top bulb needs an arc
          if (fillTop >= _.topVerticalCenter) {
              ctx.fillRect(w / 2 + Math.cos(Math.PI * 1.4) * bR, fillTop, (w / 2 + Math.cos(Math.PI * 1.6) * bR) - (w / 2 + Math.cos(Math.PI * 1.4) * bR), ((bVC + Math.sin(Math.PI * 1.4) * bR) - fillTop + 1));
          } else {
              //It does.  Fill the rectangle first
              ctx.fillRect(w / 2 + Math.cos(Math.PI * 1.4) * bR, _.topVerticalCenter, (w / 2 + Math.cos(Math.PI * 1.6) * bR) - (w / 2 + Math.cos(Math.PI * 1.4) * bR), ((bVC + Math.sin(Math.PI * 1.4) * bR) - _.topVerticalCenter + 1));
      
              //Now, draw the top arc
              ctx.beginPath();
              //get the height
              var angleHeight = fillTop - _.topVerticalCenter;
              ctx.arc(w / 2, tVC, _.topRadius, Math.asin(angleHeight / _.topRadius), Math.PI - Math.asin(angleHeight / _.topRadius), false);
              ctx.closePath();
              ctx.fill();
          }
      
      } else {
          ctx.beginPath();
          //get the height
          var fillHeight = fillTo * _.innerHeight;
          var angleHeight = _.bottomOfBulb - fillHeight - _.bulbVerticalCenter;
          ctx.arc(w / 2, bVC, bR, Math.asin(angleHeight / bR), Math.PI - Math.asin(angleHeight / bR), false); // Outer circle
          ctx.closePath();
          ctx.fill();
      }
      var majorTicksOffset = 1 / (that.settings.majorTicks - 1);
      var minorTicksOffset = majorTicksOffset / (that.settings.minorTicks + 1);
      for (var i = 0; i <= 1; i += majorTicksOffset) {
          drawTick(1-i, that.settings.centerTicks, 3, scaleValue(i));
          if (i < 1) {
            for (var j = minorTicksOffset; j < majorTicksOffset; j += minorTicksOffset) {
              drawTick(1 - (i + j) , that.settings.centerTicks, 1, scaleValue(i + j));
            }
          }
      }
      drawLabels();
    };

    function scaleValue(val) {
      var max = that.settings.max;
      var min = that.settings.min;
      return (val * (max - min)) + min;
    }

    function drawLabels() {
        var deg, fontSize, metrics, valueText;

        function formatNum(value, decimals) {
            var ret = parseFloat(value).toFixed(decimals);
            while ((decimals > 0) && ret.match(/^\d+\.(\d+)?0$/)) {
                decimals -= 1;
                ret = parseFloat(value).toFixed(decimals);
            }
            return ret;
        }

        // value text
        valueText = formatNum(getFillTo(), 2) + that.settings.unitsLabel;
        fontSize = that.settings.bulbRadius() / 2;
        styleText(that.ctx, fontSize.toFixed(0) * that.settings.scaleLabelText + 'px sans-serif');
        metrics = measureText(that.ctx, valueText);

        that.ctx.fillStyle = that.settings.color.label;
        that.ctx.textBaseline = 'middle';
        fillText(that.ctx, valueText, that.settings.w / 2 - metrics / 2, that.settings.bulbVerticalCenter);
    }

    function drawTickLabel(cx, cy, valueText, fontSize) {
        var deg, fontSize, metrics, valueText;

        function formatNum(value, decimals) {
            var ret = parseFloat(value).toFixed(decimals);
            while ((decimals > 0) && ret.match(/^\d+\.(\d+)?0$/)) {
                decimals -= 1;
                ret = parseFloat(value).toFixed(decimals);
            }
            return ret;
        }
        
        that.ctx.save();
        // value text
        valueText = formatNum(valueText, 3);
        styleText(that.ctx, fontSize.toFixed(0) * that.settings.scaleTickLabelText + 'px sans-serif');
        metrics = measureText(that.ctx, valueText);

        that.ctx.fillStyle = that.settings.color.tickLabel;
        that.ctx.textBaseline = 'middle';
        fillText(that.ctx, valueText, cx - metrics, cy);
        that.ctx.restore();
    }    

    function measureText(context, text) {
        if (context.measureText) {
            return context.measureText(text).width; //-->
        } else if (context.mozMeasureText) { //FF < 3.5
            return context.mozMeasureText(text); //-->
        }
        throw "measureText() not supported!";
    }

    function styleText(context, style) {
        context.font = style;
        context.mozTextStyle = style; // FF3
    }

    this.setupOptions = setOptions;
    this.setupOptions(options);

    this.setValue = setValue;
    
    return this;
}