import cmor,numpy

error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
  
error_flag = cmor.dataset_json("Test/common_user_input.json")

n_lev = 40
zlevs = 480.*numpy.arange(0,n_lev)+240.
zbnds = numpy.zeros((n_lev,2))

zbnds[:,0]=zlevs-240.
zbnds[:,1]=zlevs+240.

# creates 1 degree grid
cmor.load_table("CMIP6_CF3hr.json")


ialt40 = cmor.axis("alt40",units="m",coord_vals=zlevs,cell_bounds=zbnds)

itm = cmor.axis("time1",units="months since 2000")
iloc = cmor.axis("location",units="1",coord_vals=numpy.arange(2))

igrid = cmor.grid(axis_ids=[iloc,itm])

print igrid

ilat = cmor.time_varying_grid_coordinate(igrid,table_entry='latitude',units='degrees_north')
ilon = cmor.time_varying_grid_coordinate(igrid,table_entry='longitude',units='degrees_east')

#cmor.load_table("Tables/CMIP6_cf3hr.json")
ivar = cmor.variable("clcalipso",axis_ids=[igrid,ialt40],units="%")

ierr =cmor.write(ivar,numpy.ones((2,3,n_lev)),time_vals=numpy.arange(3))
ierr =cmor.write(ilat,-90.*numpy.ones((2,3,n_lev)),time_vals=numpy.arange(3),store_with=ivar)
ierr =cmor.write(ilon,180.*numpy.ones((2,3,n_lev)),time_vals=numpy.arange(3),store_with=ivar)
error_flag = cmor.close()

