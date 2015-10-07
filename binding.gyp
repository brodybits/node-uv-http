{
  "targets": [
    {
      "target_name": "node-uvhttp",
      "sources": [
        "binding.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require ('nan')\")",
        "."
      ]
    }
  ]
}