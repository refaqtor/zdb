/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Laura Schlimmer, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

FnordMetric.views["fnordmetric.metric.series.list"] = function(elem, params) {
  'use strict';

  var api_url = "/fetch";

  var url_params;
  var table;

  this.initialize = function() {
    var page = templateUtil.getTemplate("fnordmetric-metric-series-list-tpl");
    elem.appendChild(page);

    url_params = getParams(params.path);

    /** init table **/
    table = new FnordMetric.SeriesTable(elem.querySelector(".table_container"));
    table.onSort(function(col, dir) {
      updateQueryStr({
        order_col: col.key,
        order_dir: dir
      });
    });

    if (url_params.order_col && url_params.order_dir) {
      table.setSort(url_params.order_col, url_params.order_dir);
    }

    watchTimeRangePicker();
    watchFilter();
    renderHeader();
    fetchMetricSeriesList();
  };

  this.destroy = function() {

  };

/********************************* private ***********************************/

  var getParams = function(path) {
    var p = {};

    p.metric = params.route.args[1];

    var start_param = URLUtil.getParamValue(path, "start");
    if (start_param) {
      p.start = parseInt(start_param);
    }

    var end_param = URLUtil.getParamValue(path, "end");
    if (end_param) {
      p.end = parseInt(end_param);
    }

    var filter_param = URLUtil.getParamValue(path, "filter");
    if (filter_param) {
      p.filter = filter_param;
    }

    var order_col_param = URLUtil.getParamValue(path, "order_col");
    if (order_col_param) {
      p.order_col = order_col_param;
    }

    var order_dir_param = URLUtil.getParamValue(path, "order_dir");
    if (order_dir_param) {
      p.order_dir = order_dir_param;
    }

    return p;
  }

  var updateQueryStr = function(query_params) {
    var url = params.path;
    for (var k in query_params) {
      url = URLUtil.addOrModifyParam(url, k, query_params[k]);
    }

    params.app.navigateTo(url);
  }

  var watchTimeRangePicker = function() {
    var picker = elem.querySelector(
        ".fnordmetric-metric-series-list f-timerange-picker");

    var timerange = {};

    var timezone = DomUtil.getCookie("timezone");
    if (timezone) {
      timerange.timezone = timezone;
    }

    if (url_params.start && url_params.end) {
      timerange.start = url_params.start;
      timerange.end = url_params.end;
    }

    picker.initialize(timerange);

    picker.addEventListener("submit", function(e) {
      updateTimezoneCookie(this.getTimezone());
      updateQueryStr(this.getTimerange());
    }, false);
  }

  var updateTimezoneCookie = function(timezone) {
    /** set the cookie expiry date for in 5 years **/
    var d = new Date();
    d.setFullYear(d.getFullYear() + 5);
    DomUtil.setCookie("timezone", timezone, d.toUTCString());
  }

  var watchFilter = function() {
    var filter_input = elem.querySelector(".search input");
    if (url_params.filter) {
      filter_input.value = DomUtil.escapeHTML(url_params.filter);
    }

    DomUtil.onEnter(filter_input, function(e) {
      updateQueryStr({filter: filter_input.value});
    }, false);
  }

  var renderHeader = function() {
    var header = elem.querySelector(
        ".fnordmetric-metric-series-list .page_header .metric_name");
    header.innerHTML = url_params.metric;
  }

  var fetchMetricSeriesList = function() {
    var query_str = [
      "metric_id=",
      url_params.metric
    ].join("");

    var url = params.app.api_base_path + api_url + "?" + query_str;
    HTTPUtil.httpGet(url, {}, function(r) {
      if (r.status != 200) {
        params.app.renderError(
            "an error occured while loading the metric series list:",
            r.response);
        return;
      }

      var series = JSON.parse(r.response);
      table.render(series.series);
      renderChart(series.series[0]);
    });
  };

  var renderChart = function(series) {
    //FIXME make another request as soon as API is ready
    new FnordMetric.SeriesChart(elem.querySelector(".chart_pane"), {
      time: series.time,
      summary: {
        series_id: series.series_id
      },
      series: [
        {
          series_id: series.series_id,
          values: series.values,
          summaries: series.summaries,
          //REMOVEME
          title: "Current Value",
          unit: "MB/s"
          //REMOVEME END
        },
          //REMOVEME
        {
          series_id: "test",
          title: "Compare To: Yesterday",
          values: series.values.map(function(v) { return v - 1000 } ),
          unit: "MB/s"
        }
          //REMOVEME END
      ]
    });
  }

};

