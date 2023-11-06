import {
    ProducerInputQueryStore,
    ProducerInputTable,
    withProducerRepository
}                      from "@derivean/producer";
import {container}     from "@derivean/server";
import {ProducerIcon}  from "@derivean/ui";
import {IconArrowDown} from "@tabler/icons-react";
import {tx}            from "@use-pico/i18n";
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
        label={{
            title:  tx()`Producer detail`,
            header: tx()`Producer detail`,
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

        {/*<StoreProvider*/}
        {/*    store={ProducerInputQueryStore}*/}
        {/*    values={{*/}
        {/*        where: {*/}
        {/*            producerId: producer.id,*/}
        {/*        },*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <ProducerInputTable*/}

        {/*    />*/}
        {/*</StoreProvider>*/}
    </Page>;
}
