import {ProducerMenu} from "@derivean/manager";
import {
    Dependencies,
    ProducerGraph,
    ProducerPreview,
    withProducerRepository
}                     from "@derivean/producer";
import {container}    from "@derivean/server";
import {ProducerIcon} from "@derivean/ui";
import {
    Box,
    Grid,
    GridCol,
    HomeIcon,
    ListIcon,
    Nav,
    Page
}                     from "@use-pico/client";
import {t}            from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const producer = await withProducerRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<ProducerIcon/>}
        text={{
            header: t({values: producer})`Producer detail`,
        }}
        postfix={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/producer/list",
                    label: t()`Producer list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        append={<ProducerMenu
            producerId={producer.id}
            active={["/manager/producer/[id]"]}
        />}
    >
        <Box mb={"xs"}>
            <Dependencies
                producerId={producer.id}
            />
        </Box>
        <Grid pb={"md"}>
            <GridCol span={6}>
                <ProducerPreview
                    producer={producer}
                />
            </GridCol>
            <GridCol span={6}>
                <ProducerGraph
                    producerId={producer.id}
                />
            </GridCol>
        </Grid>
    </Page>;
}
