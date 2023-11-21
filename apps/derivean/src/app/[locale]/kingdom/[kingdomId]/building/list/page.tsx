import {ConstructionMenu}      from "@derivean/game";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {BuildingIcon}          from "@derivean/ui";
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

export default async function List({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t()`Kingdom building list (label)`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href:  "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon className={"min-h-max min-w-max"}/>,
                },
            ]}
        />}
        menu={<ConstructionMenu
            kingdomId={kingdom.id}
            active={["/kingdom/[kingdomId]/building/list"]}
        />}
    >
        {/*<BuildingQueryProvider*/}
        {/*    values={{}}*/}
        {/*>*/}
        {/*    /!*<KingdomBuildingTable/>*!/*/}
        {/*</BuildingQueryProvider>*/}
    </Page>;
}
