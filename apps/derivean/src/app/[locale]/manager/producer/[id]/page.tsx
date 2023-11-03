import {withProducerRepository} from "@derivean/producer";
import {container}              from "@derivean/server";
import {ProducerIcon}           from "@derivean/ui";
import {
    Breadcrumbs,
    Grid,
    GridCol,
    HomeIcon,
    ListIcon,
    Page
}                               from "@use-pico/ui";

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
        title={"manager.producer.detail"}
        withTranslation={{
            values: producer,
        }}
        postfix={<Breadcrumbs
            items={{
                "/manager":               {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                "/manager/producer/list": {
                    type:  "link",
                    href:  "/manager/producer/list",
                    label: "manager.producer.list.title",
                    icon:  <ListIcon/>,
                },
            }}
        />}
    >
        <Grid columns={2}>
            <GridCol span={1}>
                input
            </GridCol>
            <GridCol span={1}>
                output
            </GridCol>
        </Grid>
    </Page>;
}
