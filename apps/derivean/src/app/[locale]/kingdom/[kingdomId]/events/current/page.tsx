import {
    EventMenu,
    withKingdomRepository
}                  from "@derivean/kingdom";
import {container} from "@derivean/server";
import {EventIcon} from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                  from "@use-pico/client";
import {t}         from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Events({params: {kingdomId}}: Index.Props) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <Page
        icon={<EventIcon/>}
        text={{
            header: t()`Current events (label)`,
        }}
        postfix={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href:  "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
            ]}
        />}
        append={<EventMenu
            kingdomId={kingdom.id}
            active={["/kingdom/[kingdomId]/events/current"]}
        />}
    >
        Current events
    </Page>;
}
