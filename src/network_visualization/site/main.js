let last_data = ""
let resized = false;

let drag_subject = undefined
let drag_network = undefined

let networks = {}

function reset() {

    fetch("data").then((response) => {
        response.text().then((data) => {

            if (data != last_data) {


                let network_data = JSON.parse(data);


                for (let name in network_data) {
                    for (let route of network_data[name].routes) {
                        if (!route.offset) {
                            let id_links = network_data[name].routes.filter((x) => x.target == route.target && x.source == route.source)
                            let total_connections_between_nodes = id_links.length - 1;
                            let offset_range_min = -total_connections_between_nodes / 2;
                            for (let i = 0; i < id_links.length; i++) {
                                id_links[i].offset = offset_range_min + i
                            }
                        }
                    }

                    if (!networks[name])
                        networks[name] = new Network(name)

                    networks[name].set_data(network_data[name])

                }

                let num_networks = Object.keys(network_data).length

                let rows = 1
                let cols = 1
                let closest = Infinity

                for (let i = 1; i < 6; i++) {
                    for (let j = 1; j <= i; j++) {
                        let num = i * j
                        let dist = num - num_networks

                        if (dist >= 0 && dist < closest && Math.abs(j - i) <= 2) {
                            closest = dist;
                            cols = i
                            rows = j
                        }
                    }
                }

                let container = document.querySelector('#content')
                container.style["grid-template-columns"] = `repeat(${cols}, 1fr)`
                container.style["grid-template-rows"] = `repeat(${rows}, 1fr)`

                setTimeout(resize, 10)
            }

            if (!resized) {
                resized = true
                setTimeout(resize, 10)
            }


            last_data = data

        })


    })
}




function getNearest(event) {
    let subject;
    let network;

    let dist = Infinity;

    for (let network_name in networks) {
        for (let node of networks[network_name].network_data.nodes) {
            let d = Math.pow(node.x - event.subject.x, 2) + Math.pow(node.y - event.subject.y, 2)
            if (d < dist) {
                subject = node;
                network = networks[network_name]
                dist = d;
            }
        }
    }

    return [subject, network]
}

function dragstart(event) {
    [drag_subject, drag_network] = getNearest(event)

    drag_network.simulation.alphaTarget(0.3).restart()

    drag_subject.fx = null;
    drag_subject.fy = null;
}

function dragged(event) {
    drag_subject.fx = event.x;
    drag_subject.fy = event.y;
}

function dragend(event) {
    drag_network.simulation.alphaTarget(0)
}


function resize() {
    console.log("resize");
    for (let name in networks)
        networks[name].resize()
}


window.onresize = resize

setInterval(reset, 1000)
