import {ConstructionMenu}      from "@derivean/building";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {ConstructionIcon}      from "@derivean/ui";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Current({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<ConstructionIcon/>}
        text={{
            header: t()`Kingdom Building Construction (label)`,
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
        append={<ConstructionMenu
            kingdomId={kingdom.id}
            active={["/kingdom/[kingdomId]/building/construction/current"]}
        />}
    >
        {/*<StoreProvider*/}
        {/*    store={BuildingQueryStore}*/}
        {/*    values={{*/}
        {/*        filter: {*/}
        {/*            // kingdomId:*/}
        {/*        },*/}
        {/*    }}*/}
        {/*>*/}
        {/*    /!*<KingdomBuildingTable/>*!/*/}
        {/*</StoreProvider>*/}
    </Page>;
}
