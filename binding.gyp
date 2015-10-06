{
  "targets": [
    {
      "target_name": "node-uv-http",
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