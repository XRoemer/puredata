test = {}

function test.fkt1(atoms)
    pd.post("called test fkt1")
    md.start()
    outlet(1, "list", atoms)
end

function test.fkt2(args)
  pd.post("called test fkt2")
  md.start()
  outlet(1, "list", args)
end
