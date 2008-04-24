#ifndef IOTBX_PDB_INPUT_H
#define IOTBX_PDB_INPUT_H

#include <iotbx/pdb/hierarchy.h>
#include <scitbx/array_family/shared.h>
#include <scitbx/array_family/tiny.h>

namespace iotbx { namespace pdb {

  static const double anisou_factor = 1.e-4;

  static const char blank_altloc_char = ' ';

  //! Helper for looping over index ranges.
  template <typename ElementType>
  struct range_loop
  {
    range_loop() {}

    range_loop(
      af::const_ref<ElementType> const& indices,
      unsigned begin=0)
    :
      i_end(indices.end()),
      i(indices.begin()),
      end(begin)
    {}

    range_loop(
      std::vector<ElementType> const& indices,
      unsigned begin=0)
    :
      i_end(indices.size() == 0 ? 0 : (&*indices.begin()) + indices.size()),
      i(indices.size() == 0 ? 0 : &*indices.begin()),
      end(begin)
    {}

    bool
    next()
    {
      if (i == i_end) return false;
      begin = end;
      end = static_cast<unsigned>(*i++);
      size = end - begin;
      return true;
    }

    void
    skip_to_last()
    {
      if (i != i_end) i = i_end-1;
    }

    protected:
      const ElementType* i_end;
      const ElementType* i;
    public:
      unsigned begin;
      unsigned end;
      unsigned size;
  };

  //! Facilitates fast processing and comprehensive error messages.
  class line_info
  {
    public:
      const char* source_info;
      unsigned line_number;
      const char* data;
      unsigned size;

    protected:
      std::string error_source_info_;
      unsigned error_line_number_;
      std::string error_line_;
      unsigned error_column_;
      std::string error_message_;

    public:
      line_info() {}

      line_info(const char* source_info_)
      :
        source_info(source_info_),
        line_number(0),
        error_column_(0)
      {}

      template <typename StringType>
      void
      set_error(
        unsigned error_column,
        StringType error_message)
      {
        if (error_column_ != 0) return;
        error_source_info_ = (source_info ? source_info : "");
        error_line_number_ = line_number;
        error_line_ = std::string(data, size);
        error_column_ = error_column;
        error_message_ = error_message;
      }

      bool
      error_occured() const { return error_column_ != 0; }

      std::string
      format_exception_message() const;

      void
      check_and_throw_runtime_error() const
      {
        if (error_column_ == 0) return;
        throw std::runtime_error(format_exception_message());
      }

      bool
      is_blank_line() const
      {
        for(unsigned i=0;i<size;i++) {
          if (data[i] != ' ') return false;
        }
        return true;
      }

      std::string
      strip_data(unsigned start_at_column=0) const
      {
        unsigned sz = size;
        while (sz > start_at_column) {
          if (data[--sz] != ' ') {
            ++sz;
            break;
          }
        }
        if (sz <= start_at_column) return std::string();
        return std::string(data+start_at_column, data+sz);
      }
  };

  //! Detects old-style PDB files with the PDB access code in columns 73-76.
  struct columns_73_76_evaluator
  {
    const char* finding;
    bool is_old_style;
    unsigned number_of_atom_and_hetatm_lines;

    typedef af::tiny<char, 4> columns_73_76_t;

    struct columns_73_76_t_lexical_less_than
    {
      bool operator()(columns_73_76_t const& a, columns_73_76_t const& b) const
      {
        if (a[0] < b[0]) return true;
        if (a[0] > b[0]) return false;
        if (a[1] < b[1]) return true;
        if (a[1] > b[1]) return false;
        if (a[2] < b[2]) return true;
        if (a[2] > b[2]) return false;
        if (a[3] < b[3]) return true;
        return false;
      }
    };

    typedef std::map<
      columns_73_76_t,
      unsigned,
      columns_73_76_t_lexical_less_than> columns_73_76_dict_t;

    static
    bool
    is_record_type(const char* name, const char* line_data)
    {
      for(unsigned i=0;i<6;i++) {
        if (line_data[i] != name[i]) return false;
      }
      return true;
    }

    columns_73_76_evaluator() {}

    columns_73_76_evaluator(
      af::const_ref<std::string> const& lines,
      unsigned is_frequent_threshold_atom_records=1000,
      unsigned is_frequent_threshold_other_records=100);
  };

  //! Efficient processing of input atom labels.
  struct input_atom_labels
  {
    static const unsigned compacted_size = 4+1+3+2+4+1+4;
    char compacted[compacted_size];

    char*       name_begin()       { return compacted; }
    const char* name_begin() const { return compacted; }
    str4        name_small() const { return str4(name_begin(), true); }
    std::string name()       const { return std::string(name_begin(),4); }

    char*       altloc_begin()       { return compacted+4; }
    const char* altloc_begin() const { return compacted+4; }
    str1        altloc_small() const { return str1(altloc_begin(), true); }
    std::string altloc()       const { return std::string(altloc_begin(),1); }

    char*       resname_begin()       { return compacted+5; }
    const char* resname_begin() const { return compacted+5; }
    str3        resname_small() const { return str3(resname_begin(), true); }
    std::string resname()       const { return std::string(resname_begin(),3);}

    char*       confid_begin()       { return compacted+4; }
    const char* confid_begin() const { return compacted+4; }
    str4        confid_small() const { return str4(confid_begin(), true); }
    std::string confid()       const { return std::string(confid_begin(),4);}

    char*       chain_begin()       { return compacted+8; }
    const char* chain_begin() const { return compacted+8; }
    str2        chain_small() const
    {
      if (chain_begin()[0] == ' ') return str2(chain_begin()[1]);
      return str2(chain_begin(), true);
    }
    std::string chain()       const
    {
      if (chain_begin()[0] == ' ') return std::string(chain_begin()+1,1);
      return std::string(chain_begin(),2);
    }

    char*       resseq_begin()       { return compacted+10; }
    const char* resseq_begin() const { return compacted+10; }
    str4        resseq_small() const { return str4(resseq_begin(), true); }
    std::string resseq()       const { return std::string(resseq_begin(),4); }

    char*       icode_begin()       { return compacted+14; }
    const char* icode_begin() const { return compacted+14; }
    str1        icode_small() const { return str1(icode_begin(), true); }
    std::string icode()       const { return std::string(icode_begin(),1); }

    char*       resid_begin()       { return compacted+10; }
    const char* resid_begin() const { return compacted+10; }
    str5        resid_small() const { return str5(resid_begin(), true); }
    std::string resid()       const { return std::string(resid_begin(),5); }

    char*       segid_begin()       { return compacted+15; }
    const char* segid_begin() const { return compacted+15; }
    str4        segid_small() const { return str4(segid_begin(), true); }
    std::string segid()       const { return std::string(segid_begin(),4); }

    input_atom_labels() {}

    input_atom_labels(pdb::line_info& line_info)
    {
      //  7 - 11  Integer       serial   Atom serial number.
      // 13 - 16  Atom          name     Atom name.
      // 17       Character     altLoc   Alternate location indicator.
      // 18 - 20  Residue name  resName  Residue name.
      // 21 - 22                chainID  Chain identifier.
      // 23 - 26  Integer       resSeq   Residue sequence number.
      // 27       AChar         iCode    Code for insertion of residues.
      // 73 - 76  LString(4)    segID    Segment identifier, left-justified.
      extract(line_info,12,4,name_begin());
      extract(line_info,16,1,altloc_begin());
      extract(line_info,17,3,resname_begin());
      extract(line_info,20,2,chain_begin());
      extract(line_info,22,4,resseq_begin());
      extract(line_info,26,1,icode_begin());
      extract(line_info,72,4,segid_begin());
    }

    static
    void
    extract(
      pdb::line_info& line_info,
      unsigned i_begin,
      unsigned n,
      char* target)
    {
      unsigned j = 0;
      while (i_begin < line_info.size) {
        if (j == n) return;
        target[j++] = line_info.data[i_begin++];
      }
      while (j < n) target[j++] = ' ';
    }

    static
    bool
    are_equal(
      pdb::line_info& line_info,
      unsigned i_begin,
      unsigned n,
      const char* target)
    {
      unsigned j = 0;
      while (i_begin < line_info.size) {
        if (j == n) return true;
        if (target[j++] != line_info.data[i_begin++]) return false;
      }
      while (j < n) if (target[j++] != ' ') return false;
      return true;
    }

    std::string
    pdb_format() const;

    void
    check_equivalence(pdb::line_info& line_info) const;
  };

  //! Processing of PDB strings.
  class input
  {
    public:
      typedef std::map<str6, unsigned> record_type_counts_t;

      input() {}

      input(std::string const& file_name);

      input(
        const char* source_info,
        af::const_ref<std::string> const& lines);

    protected:
      void
      process(af::const_ref<std::string> const& lines);

    public:
      std::string const&
      source_info() const { return source_info_; }

      record_type_counts_t const&
      record_type_counts() const { return record_type_counts_; }

      af::shared<std::string> const&
      unknown_section() const { return unknown_section_; }

      af::shared<std::string> const&
      title_section() const { return title_section_; }

      af::shared<std::string> const&
      remark_section() const { return remark_section_; }

      af::shared<std::string> const&
      primary_structure_section() const { return primary_structure_section_; }

      af::shared<std::string> const&
      heterogen_section() const { return heterogen_section_; }

      af::shared<std::string> const&
      secondary_structure_section() const
      {
        return secondary_structure_section_;
      }

      af::shared<std::string> const&
      connectivity_annotation_section() const
      {
        return connectivity_annotation_section_;
      }

      af::shared<std::string> const&
      miscellaneous_features_section() const
      {
        return miscellaneous_features_section_;
      }

      af::shared<std::string> const&
      crystallographic_section() const { return crystallographic_section_; }

      af::shared<input_atom_labels> const&
      input_atom_labels_list() const { return input_atom_labels_list_; }

      af::shared<hierarchy::atom> const&
      atoms() const { return atoms_; }

      af::shared<std::string>
      atom_serial_number_strings() const;

      af::shared<std::string> const&
      model_ids() const { return model_ids_; }

      af::shared<std::size_t> const&
      model_indices() const { return model_indices_; }

      af::shared<std::size_t> const&
      ter_indices() const { return ter_indices_; }

      af::shared<std::vector<unsigned> > const&
      chain_indices() const { return chain_indices_; }

      af::shared<std::size_t> const&
      break_indices() const { return break_indices_; }

      af::shared<std::string> const&
      connectivity_section() const { return connectivity_section_; }

      af::shared<std::string> const&
      bookkeeping_section() const { return bookkeeping_section_; }

      af::shared<std::size_t>
      model_atom_counts() const;

      //! not const because atom parents are modified.
      hierarchy::root
      construct_hierarchy(
        bool residue_group_post_processing=true);

    protected:
      std::string source_info_;
      record_type_counts_t record_type_counts_;
      af::shared<std::string> unknown_section_;
      af::shared<std::string> title_section_;
      af::shared<std::string> remark_section_;
      af::shared<std::string> primary_structure_section_;
      af::shared<std::string> heterogen_section_;
      af::shared<std::string> secondary_structure_section_;
      af::shared<std::string> connectivity_annotation_section_;
      af::shared<std::string> miscellaneous_features_section_;
      af::shared<std::string> crystallographic_section_;
      af::shared<input_atom_labels> input_atom_labels_list_;
      af::shared<hierarchy::atom> atoms_;
      af::shared<std::string> model_ids_;
      af::shared<std::size_t> model_indices_;
      af::shared<std::size_t> ter_indices_;
      af::shared<std::vector<unsigned> > chain_indices_;
      af::shared<std::size_t> break_indices_;
      af::shared<unsigned>    break_record_line_numbers;
      af::shared<std::string> connectivity_section_;
      af::shared<std::string> bookkeeping_section_;
  };

}} // namespace iotbx::pdb

#endif // IOTBX_PDB_INPUT_H
