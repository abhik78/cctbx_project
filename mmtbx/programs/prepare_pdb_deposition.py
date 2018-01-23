# -*- coding: utf-8 -*-
from __future__ import division, print_function

import os

import iotbx.pdb
import mmtbx.model
from libtbx import group_args
from libtbx.program_template import ProgramTemplate

# just for testing
import libtbx.phil
program_citations = libtbx.phil.parse('''
citation {
  article_id = hhpred
  authors = Söding J
  title = Protein homology detection by HMM-HMM comparison.
  journal = Bioinformatics
  volume = 21
  pages = 951-60
  year = 2005
  doi_id = "10.1093/bioinformatics/bti125"
  pmid = 15531603
  external = True
}

citation {
  article_id = erraser
  authors = Chou FC, Sripakdeevong P, Dibrov SM, Hermann T, Das R
  title = Correcting pervasive errors in RNA crystallography through enumerative structure prediction.
  journal = Nat Methods
  volume = 10
  pages = 74-6
  year = 2012
  doi_id = "10.1038/nmeth.2262"
  pmid = 23202432
  external = True
}

''')
# end testing

# =============================================================================
class Program(ProgramTemplate):

  description = '''
Program for preparing model and data files for depostion into the Proten Data
Bank\n

Minimum required data:
  Model file
  Sequence file

The sequence file should have a sequence for each chain in the model file.

Currently, this program only combines the model and sequence into a single
mmCIF file. More functionality is planned.
'''

  master_phil_str = '''
sequence_alignment {
  include scope mmtbx.validation.sequence.master_phil
}
'''

  # just for testing
  citations = program_citations
  known_article_ids = ['phenix', 'phenix.polder']
  # end testing

  # ---------------------------------------------------------------------------
  def validate(self):
    print('Validating inputs', file=self.logger)
    self.data_manager.has_models()
    self.data_manager.has_sequences()

  # ---------------------------------------------------------------------------
  def run(self):
    model = self.data_manager.get_model()
    self.cif_blocks = list()

    # sequence block
    print ('Creating mmCIF block for sequence', file=self.logger)
    sequence = self.data_manager.get_sequence()
    hierarchy = model._pdb_hierarchy
    seq_block = hierarchy.as_cif_block_with_sequence(
      sequence, crystal_symmetry=model.crystal_symmetry(),
      alignment_params=self.params.sequence_alignment)
    self.cif_blocks.append(seq_block)

    # create additional cif blocks?

    # add cif blocks together
    print ('Creating complete mmCIF', file=self.logger)
    self.cif_model = model.model_as_mmcif(additional_blocks=self.cif_blocks)

    # write output file
    self.output_file = os.path.splitext(
      self.data_manager.get_default_model_name())[0] + '.deposit.cif'
    print ('Writing mmCIF', file=self.logger)
    print ('  Output file = %s' % self.output_file, file=self.logger)
    with open(self.output_file, 'wb') as f:
      f.write(self.cif_model)

    # update data manager for any downstream applications
    pdb_input = iotbx.pdb.input(self.output_file)
    model = mmtbx.model.manager(model_input=pdb_input, log=self.logger)
    self.data_manager.add_model(self.output_file, model)
    self.data_manager.set_default_model(self.output_file)

  # ---------------------------------------------------------------------------
  def get_results(self):
    return group_args(output_file=self.output_file,
                      cif_model=self.cif_model)

# =============================================================================
# end