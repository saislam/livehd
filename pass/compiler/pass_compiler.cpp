// This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#include "pass_compiler.hpp"

static Pass_plugin sample("pass_compiler", Pass_compiler::setup);


void Pass_compiler::setup() {
  Eprp_method m1("pass.compiler", "LiveHD multi-HDLs compilation, default language: Pyrope", &Pass_compiler::compile);
  m1.add_label_optional("path",   "lgraph path", "lgdb");
  m1.add_label_optional("files",  "files to process (comma separated)");
  m1.add_label_optional("firrtl", "is firrtl front-end");
  m1.add_label_optional("top",    "specify the top module");
  m1.add_label_optional("odir",   "output directory", ".");
  m1.add_label_optional("gviz",   "dump graphviz");

  register_pass(m1);
}

Pass_compiler::Pass_compiler(const Eprp_var &var) : Pass("pass.compiler", var) {}


bool Pass_compiler::check_option_gviz(Eprp_var &var) { 
  bool gviz_en; 
  if (var.has_label("gviz")) { 
    auto gv = var.get("gviz"); 
    gviz_en = gv != "false" && gv != "0"; 
  } else { 
    gviz_en = false; 
  } 
  return gviz_en; 
} 

std::string Pass_compiler::check_option_top(Eprp_var &var) { 
  std::string top; 
  if (var.has_label("top")) { 
    top = var.get("top"); 
  }  
  return top; 
} 


bool Pass_compiler::check_option_firrtl(Eprp_var &var) { 
  bool is_firrtl; 
  if (var.has_label("firrtl")) { 
    auto fir = var.get("firrtl"); 
    is_firrtl = fir != "false" && fir != "0"; 
  } else { 
    is_firrtl = false; 
  } 
  return is_firrtl; 
} 



void Pass_compiler::compile(Eprp_var &var) {
  Pass_compiler pc(var);
  auto path      = pc.get_path(var);
  auto odir      = pc.get_odir(var);
  auto top       = pc.check_option_top(var);
  bool gviz      = pc.check_option_gviz(var);
  bool is_firrtl = pc.check_option_firrtl(var);

  Lcompiler compiler(path, odir, top, gviz);
  fmt::print("top module_name is:{}\n", top);

  if (var.lnasts.empty()) {
    auto files = pc.get_files(var);
    if (files.empty()) {
      Pass::warn("nothing to compile. no files or lnast");
      return;
    }

    for (auto f : absl::StrSplit(files, ',')) 
      Pass::warn("todo: start from prp parser:{}", f);
  } 


  if (is_firrtl) {
    I(top != "", "firrtl front-end must specify the top firrtl name!");
    auto lg = LGraph::create(path, top, var.lnasts.front()->get_source());
    setup_firmap_library(lg);   
    firrtl_compilation(var, compiler);
  } else {
    pyrope_compilation(var, compiler);
  }

  auto lgs = compiler.wait_all();
  var.add(lgs);
  return;
}

void Pass_compiler::pyrope_compilation(Eprp_var &var, Lcompiler &compiler) {
  for (const auto &lnast : var.lnasts) 
    compiler.add_pyrope(lnast);

  compiler.global_io_connection();  
  compiler.global_bitwidth_inference();  
}


void Pass_compiler::firrtl_compilation(Eprp_var &var, Lcompiler &compiler) {
    for (const auto &lnast : var.lnasts) 
      compiler.add_firrtl(lnast);
    
    compiler.global_io_connection();  
    compiler.global_firrtl_bits_analysis_map();
    compiler.global_bitwidth_inference();  
}


void Pass_compiler::setup_firmap_library(LGraph *lg) {
  auto &lg_fir_add = lg->ref_library()->setup_sub("__fir_add", "-");
  lg_fir_add.add_input_pin("A");
  lg_fir_add.add_input_pin("B");
  lg_fir_add.add_output_pin("Y");
  

  auto &lg_fir_sub = lg->ref_library()->setup_sub("__fir_sub", "-");
  lg_fir_sub.add_input_pin("A");
  lg_fir_sub.add_input_pin("B");
  lg_fir_sub.add_output_pin("Y");

  lg->ref_library()->sync();
}
