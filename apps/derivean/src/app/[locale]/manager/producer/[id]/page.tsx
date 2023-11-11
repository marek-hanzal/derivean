import {
    ProducerInputQueryStore,
    ProducerInputTable,
    ProducerOutputQueryStore,
    ProducerOutputTable,
    withProducerRepository
}                      from "@derivean/producer";
import {container}     from "@derivean/server";
import {ProducerIcon}  from "@derivean/ui";
import {IconArrowDown} from "@tabler/icons-react";
import {
    t,
    tx
}                      from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    Center,
    HomeIcon,
    ListIcon,
    Page,
    WithIcon
}                      from "@use-pico/ui";

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
            title: tx({values: producer})`Producer detail (title)`,
            header: t({values: producer})`Producer detail`,
        }}
        postfix={<Breadcrumbs
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
    >
        <StoreProvider
            store={ProducerInputQueryStore}
            values={{
                where: {
                    producerId: producer.id,
                },
            }}
        >
            <ProducerInputTable
                hidden={["producerId"]}
                producerId={producer.id}
            />
        </StoreProvider>

        <Center my={"xl"}>
            <WithIcon
                color={"gray.5"}
                size={64}
                icon={<IconArrowDown size={64}/>}
            />
        </Center>

        <StoreProvider
            store={ProducerOutputQueryStore}
            values={{
                where: {
                    producerId: producer.id,
                },
            }}
        >
            <ProducerOutputTable
                hidden={["producerId"]}
                producerId={producer.id}
            />
        </StoreProvider>

        <h1>Show required buildings (resources) to run this producer + graph + production time</h1>
    </Page>;
}
