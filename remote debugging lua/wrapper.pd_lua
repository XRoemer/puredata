local Wrapper = pd.Class:new():register("wrapper")
md = require('mobdebug')

function Wrapper:initialize(name, atoms)
  self.inlets =  1
  self.outlets = 2
  return true
end

function Wrapper:finalize()
  pd.post("finalize")
end

function Wrapper:in_1(sel,atoms)
  Wrapper.self = self
  
  if atoms[1] == 'dofile' then
    dofile(atoms[2])
  else
    _ENV[atoms[1]][atoms[2]](split_table(atoms))
  end
end

function outlet(nr, type, msg)
  Wrapper.self:outlet(nr, type, msg)
end


function split_table(tbl)
  local new_tbl = {}
  for key,value in pairs(tbl) do 
    if key > 2 then
      table.insert(new_tbl, value)
    end
  end
  return new_tbl
end

