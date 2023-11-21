import {ProducerMenu}           from "@derivean/manager";
import {withProducerRepository} from "@derivean/producer";
import {container}              from "@derivean/server";
import {ProducerIcon}           from "@derivean/ui";
import {
    HomeIcon,
    ListIcon,
    Nav,
    Page
}                               from "@use-pico/client";
import {
    t,
    tv
}                               from "@use-pico/translator";

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
            header: tv(producer)`Producer detail`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/producer/list",
                    label: t`Producer list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        menu={<ProducerMenu
            producerId={producer.id}
            active={["/manager/producer/[id]/pipeline"]}
        />}
    >
        {/*<Box mb={"xs"}>*/}
        {/*    <Dependencies*/}
        {/*        producerId={producer.id}*/}
        {/*    />*/}
        {/*</Box>*/}

        {/*<Pipeline producerId={producer.id}/>*/}
    </Page>;
};
