/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#ifndef _FNORDMETRIC_BARCHART_H
#define _FNORDMETRIC_BARCHART_H
#include <stdlib.h>
#include <assert.h>
#include "domain.h"
#include "../base/series.h"

namespace fnordmetric {
namespace ui {
class Domain;
class Canvas;

/**
 * This draws a horizontal or vertical bar/column chart. For two dimensional
 * series this draws one bars for each point in the series with label X and
 * height Y. For three dimensional series this draws one bar for each point in
 * the series with label X, lower bound Y and upper bound Z.
 *
 * If points share the same labels in all series they will be shown as grouped
 * bars by series. If stacked is true, all bars in a group will be stacked on
 * top of each other.
 *
 * OPTIONS
 *
 *   height             = int
 *   width              = int
 *   orientation        = {horizontal,vertical}, default: horizontal
 *   stacked            = {on,off}, default: off
 *   show_labels        = {inside,outside,auto,off}, default: auto
 *   rotate_labels      = {on,off,auto}, default: auto
 *   label_color        = color
 *
 */
class BarChart {
public:
  enum kBarChartOrientation {
    O_VERTICAL,
    O_HORIZONTAL
  };

  constexpr static const double kBarPadding = 0.2f; // FIXPAUL make configurable

  /**
   * Create a new bar chart with an explicit y domain
   *
   * @param canvas the canvas to draw this chart on. does not transfer ownership
   * @param orientation one of {O_HORIZNTAL,O_VERTICAL}. default is horizontal
   * @param the y/value domain. does not transfer ownership
   */
  BarChart(
      Canvas* canvas,
      NumericalDomain* y_domain = nullptr,
      kBarChartOrientation orientation = O_HORIZONTAL);

  /**
   * Add a (x: string, y: double) series. This will draw one bar for each point
   * in the series where x is the label of the bar and y is the height of the
   * bar
   *
   * @param series the series to add. does not transfer ownership
   */
  void addSeries(Series2D<std::string, double>* series);

  /**
   * Add a (x: double, y: double) series. This will draw one bar for each point
   * in the series where x is the label of the bar and y is the height of the
   * bar
   *
   * @param series the series to add. does not transfer ownership
   */
  void addSeries(Series2D<double, double>* series);

  /**
   * Add a (x: string, y: double, z: double) series. This will draw one bar for
   * each point in the series where x is the label of the bar, y is the lower
   * bound of the bar and z is the upper bound of the bar.
   *
   * @param series the series to add. does not transfer ownership
   */
  //void addSeries(Series2D<std::string, double, double>* series);

  /**
   * Add a (x: double, y: double, z: double) series. This will draw one bar for
   * each point in the series where x is the label of the bar, y is the lower
   * bound of the bar and z is the upper bound of the bar.
   *
   * @param series the series to add. does not transfer ownership
   */
  //void addSeries(Series3D<double, double, double>* series);

  //void draw(ChartRenderTarget* target) override;

protected:
  /*
  struct BarData {
    query::SValue x;
    std::vector<std::pair<double, double>> ys;
  };
*/

  //void drawVerticalBars(RenderTarget* target);
  //void drawHorizontalBars(RenderTarget* target);
  void prepareData();

  //std::pair<double, double> scaleValue(
  //    const query::SValue* value, 
  //    const Domain* domain) const;

  kBarChartOrientation orientation_;
  NumericalDomain* y_domain_;
  bool stacked_;
  //std::vector<Series2D*> series_;
};

}
}
#endif