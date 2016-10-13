FnordMetric.views["fnordmetric.metric"] = function(elem, params) {
  var api_path = "/list_series";
  var path = params.path;
  var url_params;
  var view_cfg;
  var viewport;

  this.initialize = function() {
    url_params = getParams(params.path);
    var page = templateUtil.getTemplate("fnordmetric-metric-detail-tpl");

    DomUtil.handleLinks(page, params.app.navigateTo);
    DomUtil.replaceContent(elem, page);
    viewport = new Viewport(elem.querySelector(".view_content"));

    view_cfg = new FnordMetric.MetricTableViewConfig(url_params);
    render();
    fetchData();
  }

  var updatePath = function() {
    params.app.navigateTo(
        params.route.args[0] + "?" + URLUtil.buildQueryString(view_cfg.getParamList()));
  }

  var getParams = function() {
    var p = {};

    p.metric = params.route.args[1];
    var cfg_param = URLUtil.getParamValue(path, "cfg");
    if (cfg_param) {
      p.config = cfg_param;
    }

    var offset_param = URLUtil.getParamValue(path, "offset");
    if (offset_param) {
      p.offset = offset_param;
    }

    return p;
  }

  var fetchData = function() {
    var url = params.app.api_base_path + api_path + "?metric_id=test"; //FIXME 
    HTTPUtil.httpGet(url, {}, function(r) {
      if (r.status != 200) {
        params.app.renderError(
            "an error occured while loading the metric list:",
            r.response);
        return;
      }

      var series = JSON.parse(r.response);
      renderView(series);
    });
    //updateTable(result);
    //renderPagination(result.rows.length);
  }

  var render = function() {
    renderHeader();
    // renderView(); --> initView
    //renderFilterList();
    renderTimerangeControl();
    renderEmbedControl();
  };

  var renderHeader = function(metric) {
    var header = elem.querySelector(
        ".fnordmetric-metric-table .page_header .metric_name");
    header.innerHTML = url_params.metric;

    /* handle view controls */
    elem.querySelector(".fnordmetric-metric-table .view_control").setAttribute(
        "data-view", view_cfg.getValue("view"));

    /* switch to table view */
    var table_view_ctrl = elem.querySelector(
        ".fnordmetric-metric-table .view_control .table_view");
    table_view_ctrl.addEventListener("click", function(e) {
      view_cfg.updateValue("view", "table");
      updatePath();
    }, false);

    /* switch to timeseries view */
    var timeseries_view_ctrl = elem.querySelector(
        ".fnordmetric-metric-table .view_control .timeseries_view");
    timeseries_view_ctrl.addEventListener("click", function(e) {
      view_cfg.updateValue("view", "timeseries");
      updatePath();
    }, false);
  };

  var renderTimerangeControl = function() {
    var dropdown = elem.querySelector(
        ".fnordmetric-metric-table .control dropdown.timerange");
  }

  var renderView = function(results) {
    //TODO enable / disable current view icon in header
    var view;
    var view_opts = {
      data: results,
      view_cfg: view_cfg
    };

    switch (view_cfg.getValue("view")){
      case "table":
        view = FnordMetric.views["fnordmetric.metric.table"];
        break;

      case "timeseries":
        view = FnordMetric.views["fnordmetric.metric.timeseries"];
        break;

      default:
        view = FnordMetric.views["fnordmetric.404"];
        break;
    }

    viewport.setView(view, view_opts);
  }

  //var renderFilterList = function() {
  //  var filter_strs = view_cfg.getValue("filter") ? view_cfg.getValue("filter") : [];
  //  var filter_ctrl = new FnordMetric.MetricTableFilter(
  //      elem.querySelector(".fnordmetric-metric-table z-modal.filter"));

  //  /* add a new filter */
  //  var add_link = elem.querySelector(
  //      ".fnordmetric-metric-table .sidebar a.add_filter");
  //  add_link.addEventListener("click", function(e) {
  //    filter_ctrl.render();
  //  }, false);
  //  filter_ctrl.onSubmit(function(filter_str) {
  //    console.log(filter_str);
  //    filter_strs.push(filter_str);
  //    view_cfg.updateValue("filter", filter_strs);
  //    updatePath();
  //  });

  //  /* render exisiting filter list */
  //  var flist_elem = elem.querySelector(
  //      ".fnordmetric-metric-table .sidebar .filter_list");
  //  var felem_tpl = templateUtil.getTemplate(
  //      "fnordmetric-metric-table-filter-list-elem-tpl");

  //  filter_strs.forEach(function(f) {
  //    var felem = felem_tpl.cloneNode(true);
  //    felem.querySelector(".filter_value").innerHTML = f;

  //    felem.querySelector(".edit").addEventListener("click", function(e) {
  //      filter_ctrl.render(f);
  //    });

  //    flist_elem.appendChild(felem);
  //  });

  //  /* change an existing filter */
  //  filter_ctrl.onChange(function(new_filter_str, old_filter_str) {
  //    var idx = filter_strs.indexOf(old_filter_str);
  //    if (idx > -1) {
  //      filter_strs[idx] = new_filter_str;
  //    }

  //    view_cfg.updateValue("filter", filter_strs);
  //    updatePath();
  //  });

  //  /* remove an exisiting filter */
  //  filter_ctrl.onDelete(function(old_filter_str) {
  //    var idx = filter_strs.indexOf(old_filter_str);
  //    if (idx > -1) {
  //      filter_strs.splice(idx, 1);
  //    }
  //    view_cfg.updateValue("filter", filter_strs);
  //    updatePath();
  //  });
  //}

  var renderEmbedControl = function() {
    elem.querySelector(".fnordmetric-metric-table .control.embed")
        .addEventListener("click", function() {
          fEmbedPopup(elem, "").render();
        }, false);
  }
}

