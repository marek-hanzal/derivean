import {BuildingQueryStore}    from "@derivean/building";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {BuildingIcon}          from "@derivean/ui";
import {tx}                    from "@use-pico/i18n";
import {StoreProvider}         from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                              from "@use-pico/ui";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Construction({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: tx()`Kingdom Building Construction (label)`,
        }}
        postfix={<Breadcrumbs
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
    >
        <StoreProvider
            store={BuildingQueryStore}
            values={{
                filter: {
                    // kingdomId:
                },
            }}
        >
            {/*<KingdomBuildingTable/>*/}
        </StoreProvider>
    </Page>;
}
