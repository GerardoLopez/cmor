import cmor

def path_test():
    cmor.setup(inpath='Test',netcdf_file_action=cmor.CMOR_REPLACE)

    cmor.dataset('abrupt2xCO2', 'ukmo', 'HadCM3', '360_day',
                 institute_id="PCMDI",
                 parent_experiment_id="abrupt4xCO2",
                 parent_experiment_rip="r1i1p1",
                 branch_time=201201,
                 contact="LLNL",
                 model_id='HadCM3',forcing='N/A')
    
    table='Tables/CMIP6_Amon_json'
    cmor.load_table(table)
    axes = [ {'table_entry': 'time',
              'units': 'days since 2000-01-01 00:00:00',
              'coord_vals': [15],
              'cell_bounds': [0, 30]
              },
             {'table_entry': 'latitude',
              'units': 'degrees_north',
              'coord_vals': [0],
              'cell_bounds': [-1, 1]},             
             {'table_entry': 'longitude',
              'units': 'degrees_east',
              'coord_vals': [90],
              'cell_bounds': [89, 91]},
             ]
              
    axis_ids = list()
    for axis in axes:
        axis_id = cmor.axis(**axis)
        axis_ids.append(axis_id)
    varid = cmor.variable('ts', 'K', axis_ids)
    cmor.write(varid, [273])
    path=cmor.close(varid, file_name=True)

    print "Saved file: ",path

if __name__ == '__main__':
    path_test()
