with open("some_other_file", "r") as f:
  out = ""
  prev_dash = False
  for line in f:
    if "ANON" in line or "FILE" in line:
      continue
    if "---" in line and prev_dash:
      out=out[:-3]
      out += "\n"
      continue
    if "---" in line:
      prev_dash = True
      continue
    prev_dash = False
    if line == "\n":
      continue
    out += str([int(s) for s in line.split() if s.isdigit()][0]) + ","
  print out[:-3]
