import Git: git

run(`$(git()) config --global user.name "Johnathan Bizzano"`)
run(`$(git()) config --global user.email "bizzanoj@my.erau.edu"`)

const repo_user = "HyperSphereStudio"

const VERSION = "0.4.3"
const deploy_local = false
const tarball_name = "build_tarballs"

#Delete Past Product Directory
isdir("products") && rm("products", recursive=true)

if deploy_local
    @info "Deployment: local"
    repo = "local"
else
    @info "Deployment: github"
    repo = "$repo_user/libsimplecommunicationencoder_jll"
end

run(`julia -t 8 $tarball_name.jl --debug --verbose --deploy=$repo`)