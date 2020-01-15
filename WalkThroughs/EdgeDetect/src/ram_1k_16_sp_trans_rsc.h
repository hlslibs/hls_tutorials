/**************************************************************************
 *                                                                        *
 *  Edge Detect Design Walkthrough for HLS                                *
 *                                                                        *
 *  Software Version: 1.0                                                 *
 *                                                                        *
 *  Release Date    : Tue Jan 14 15:40:43 PST 2020                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 1.0.0                                               *
 *                                                                        *
 *  Copyright 2020, Mentor Graphics Corporation,                          *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *  
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
#ifndef __INCLUDED_ram_1k_16_sp_trans_rsc_H__
#define __INCLUDED_ram_1k_16_sp_trans_rsc_H__
#include <mc_transactors.h>

template < 
  int data_width
  ,int addr_width
  ,int depth
>
class ram_1k_16_sp_trans_rsc : public mc_wire_trans_rsc_base<data_width,depth>
{
public:
  sc_in< sc_lv<addr_width> >   addr;
  sc_in< sc_lv<data_width> >   d;
  sc_in< sc_logic >   chip_en;
  sc_in< sc_logic >   wr_en;
  sc_in< sc_logic >   rd_en;
  sc_in< bool >   clk;
  sc_out< sc_lv<data_width> >   q;

  typedef mc_wire_trans_rsc_base<data_width,depth> base;
  MC_EXPOSE_NAMES_OF_BASE(base);

  SC_HAS_PROCESS( ram_1k_16_sp_trans_rsc );
  ram_1k_16_sp_trans_rsc(const sc_module_name& name, bool phase, double clk_skew_delay=0.0)
    : base(name, phase, clk_skew_delay)
    ,addr("addr")
    ,d("d")
    ,chip_en("chip_en")
    ,wr_en("wr_en")
    ,rd_en("rd_en")
    ,clk("clk")
    ,q("q")
    ,_is_connected_port_0(true)
    ,_is_connected_port_0_messaged(false)
  {
    SC_METHOD(at_active_clock_edge);
    sensitive << (phase ? clk.pos() : clk.neg());
    this->dont_initialize();

    MC_METHOD(clk_skew_delay);
    this->sensitive << this->_clk_skew_event;
    this->dont_initialize();
  }

  virtual void start_of_simulation() {
    if ((base::_holdtime == 0.0) && this->get_attribute("CLK_SKEW_DELAY")) {
      base::_holdtime = ((sc_attribute<double>*)(this->get_attribute("CLK_SKEW_DELAY")))->value;
    }
    if (base::_holdtime > 0) {
      std::ostringstream msg;
      msg << "ram_1k_16_sp_trans_rsc CLASS_STARTUP - CLK_SKEW_DELAY = "
        << base::_holdtime << " ps @ " << sc_time_stamp();
      SC_REPORT_INFO(this->name(), msg.str().c_str());
    }
    reset_memory();
  }

  virtual void inject_value(int addr, int idx_lhs, int mywidth, sc_lv_base& rhs, int idx_rhs) {
    this->set_value(addr, idx_lhs, mywidth, rhs, idx_rhs);
  }

  virtual void extract_value(int addr, int idx_rhs, int mywidth, sc_lv_base& lhs, int idx_lhs) {
    this->get_value(addr, idx_rhs, mywidth, lhs, idx_lhs);
  }

private:
  void at_active_clock_edge() {
    base::at_active_clk();
  }

  void clk_skew_delay() {
    this->exchange_value(0);
    if ( chip_en.read() == 0 ) return; //  everything stalls if enable is inactive

    if (addr.get_interface())
      _addr = addr.read();
    else {
      _is_connected_port_0 = false;
    }
    if (d.get_interface())
      _d = d.read();
    else {
      _is_connected_port_0 = false;
    }
    if (chip_en.get_interface())
      _chip_en = chip_en.read();
    if (wr_en.get_interface())
      _wr_en = wr_en.read();
    if (rd_en.get_interface())
      _rd_en = rd_en.read();

    //  Write
    int _w_addr_port_0 = -1;
    if ( _is_connected_port_0 && (_wr_en==1)) {
      _w_addr_port_0 = get_addr(_addr, "addr");
      if (_w_addr_port_0 >= 0)
        inject_value(_w_addr_port_0, 0, data_width, _d, 0);
    }
    if( !_is_connected_port_0 && !_is_connected_port_0_messaged) {
      std::ostringstream msg;msg << "port_0 is not fully connected and writes on it will be ignored";
      SC_REPORT_WARNING(this->name(), msg.str().c_str());
      _is_connected_port_0_messaged = true;
    }

    //  Sync Read
    if ((_rd_en==1)) {
      const int addr = get_addr(_addr, "addr");
      if (addr >= 0)
      {
        if (addr==_w_addr_port_0) {
          sc_lv<data_width> dc; // X
          _q = dc;
        }
        else
          extract_value(addr, 0, data_width, _q, 0);
      }
      else { 
        sc_lv<data_width> dc; // X
        _q = dc;
      }
    }
    if (q.get_interface())
      q = _q;
    this->_value_changed.notify(SC_ZERO_TIME);
  }

  int get_addr(const sc_lv<addr_width>& addr, const char* pin_name) {
    if (addr.is_01()) {
      const int cur_addr = addr.to_uint();
      if (cur_addr < 0 || cur_addr >= depth) {
#ifdef CCS_SYSC_DEBUG
        std::ostringstream msg;
        msg << "Invalid address '" << cur_addr << "' out of range [0:" << depth-1 << "]";
        SC_REPORT_WARNING(pin_name, msg.str().c_str());
#endif
        return -1;
      } else {
        return cur_addr;
      }
    } else {
#ifdef CCS_SYSC_DEBUG
      std::ostringstream msg;
      msg << "Invalid Address '" << addr << "' contains 'X' or 'Z'";
      SC_REPORT_WARNING(pin_name, msg.str().c_str());
#endif
      return -1;
    }
  }

  void reset_memory() {
    this->zero_data();
    _addr = sc_lv<addr_width>();
    _d = sc_lv<data_width>();
    _chip_en = SC_LOGIC_X;
    _wr_en = SC_LOGIC_X;
    _rd_en = SC_LOGIC_X;
    _is_connected_port_0 = true;
    _is_connected_port_0_messaged = false;
  }

  sc_lv<addr_width>  _addr;
  sc_lv<data_width>  _d;
  sc_logic _chip_en;
  sc_logic _wr_en;
  sc_logic _rd_en;
  sc_lv<data_width>  _q;
  bool _is_connected_port_0;
  bool _is_connected_port_0_messaged;
};
#endif // ifndef __INCLUDED_ram_1k_16_sp_trans_rsc_H__


