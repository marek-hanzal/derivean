import {withBuildingRepository} from "@derivean/building";
import {BuildingMenu}           from "@derivean/manager";
import {container}              from "@derivean/server";
import {BuildingIcon}           from "@derivean/ui";
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

export default async function Requirement({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: tv(building)`Building requirement`,
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
                    href:  "/manager/building/list",
                    label: t`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        menu={<BuildingMenu
            buildingId={building.id}
            active={["/manager/building/[id]/requirement"]}
        />}
    >
        {/*<StoreProvider*/}
        {/*    store={BuildingRequirementQueryStore}*/}
        {/*    values={{*/}
        {/*        where: {*/}
        {/*            buildingId: building.id,*/}
        {/*        },*/}
        {/*    }}*/}
        {/*>*/}
        {/*    <BuildingRequirementTable*/}
        {/*        hidden={["building"]}*/}
        {/*        buildingId={building.id}*/}
        {/*    />*/}
        {/*</StoreProvider>*/}
    </Page>;
};
